#include "common.h"

#include "OutputProcessor.h"

void COutputProcessor::DoOutput() {
    std::ostream * pout = &std::cout;
    std::unique_ptr < std::ofstream > pout_file;
    if ( !DUMP_TO_STDOUT ) {
        pout_file = std::make_unique < std::ofstream >(
            filename.empty()
            ?
            std::to_string( m_stats_ts ) + "-" + std::to_string( m_min_unprocessed_time ) + ".csv"
            :
            filename
        );
        pout = pout_file.get();
    }

    if ( DUMP_TO_STDOUT ) {
        *pout << "[ " << to_stream( m_stats_ts ) << " .. " << to_stream( m_min_unprocessed_time ) << " )" << std::endl;
    }

    std::lock_guard < std::mutex > lock( m_stats_item->GetMutex() );
    auto result_map = m_stats_item->GetStats();

    std::set < std::string > result_codes;
    for ( const auto & stats : std::views::values( result_map ) ) {
        const auto & request_result_codes = std::views::keys( stats );
        result_codes.insert( request_result_codes.begin(), request_result_codes.end() );
    }

    const char * csv_separator = ";";//"\t";

    *pout << "request";
    for ( const auto & result_code : result_codes ) {
        *pout << csv_separator << result_code;
    }
    for ( auto & [ request, stats ] : result_map ) {
        *pout << std::endl;
        *pout << request;
        for ( const auto & result_code : result_codes ) {
            *pout << csv_separator << stats[ result_code ];
        }
    }

    *pout << std::endl;

}

bool COutputProcessor::OutputStats( const bool bForceOutput ) {

    bool bResult = false;
    time_t current_time = CAggregatedStatsCollection::GetQuantizedTime( time( nullptr ) );
    bool bShouldWait = false;

    while (
        !bShouldWait
        &&
        stats.GetOldest( m_stats_item, m_stats_ts )
        &&
        (
            m_stats_ts < current_time
            ||
            bForceOutput
        )
    ) {

        m_min_unprocessed_time = CAggregatedStatsCollection::GetQuantizedTime( m_stats_ts, +1 );

        if ( time_t ts = 0; filling_line_buffers.GetOldestTimestamp( ts ) && ts < m_min_unprocessed_time ) {
            DEBUG_OUTPUT && std::cout << "Waiting for unfilled line buffer at " << ts - m_min_unprocessed_time << " seconds" << std::endl;
            bShouldWait = true;
        }

        if ( time_t ts = 0; ready_line_buffers.GetOldestTimestamp( ts ) && ts < m_min_unprocessed_time ) {
            DEBUG_OUTPUT && std::cout << "Waiting for unparsed line buffer at " << ts - m_min_unprocessed_time << " seconds" << std::endl;
            bShouldWait = true;
        }

        if ( time_t ts = 0; response_map.GetOldestTimestamp( ts ) && ts < m_min_unprocessed_time ) {
            DEBUG_OUTPUT && std::cout << "Waiting for unparsed response buffer at " << ts - m_min_unprocessed_time << " seconds" << std::endl;
            bShouldWait = true;
        }

        if ( !bShouldWait ) {
            bResult = true;
            DoOutput();
            stats.RemoveItem( m_stats_item );
        }
    }

    return bResult;
}

void COutputProcessor::Run( const std::stop_token & stoken ) {
    bool bHadOutput = false;
    while ( true ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        COutputProcessor op;
        if ( op.OutputStats( stoken.stop_requested() && !bHadOutput ) ) {
            bHadOutput = true;
        }
        if ( stoken.stop_requested() ) {
            break;
        }
    }
}
