#include "common.h"

#include "OutputProcessor.h"

COutputProcessor::COutputProcessor( PContext context )
    : m_context( std::move( context ) )
{
}

void COutputProcessor::DoOutput() {

    if ( m_context->DUMP_TO_STDOUT ) {
        std::cout << "[ " << to_stream( m_stats_ts ) << " .. " << to_stream( m_min_unprocessed_time ) << " )" << std::endl;
    }

    std::string s;

    std::lock_guard < std::mutex > lock( m_stats_item->GetMutex() );
    auto result_map = m_stats_item->GetStats();

    std::set < std::string > result_codes;
    for ( const auto & stats : std::views::values( result_map ) ) {
        const auto & request_result_codes = std::views::keys( stats );
        result_codes.insert( request_result_codes.begin(), request_result_codes.end() );
    }

    const char * csv_separator = ";";//"\t";

    s += "request";
    for ( const auto & result_code : result_codes ) {
        s += csv_separator;
        s += result_code;
    }
    for ( auto & [ request, stats ] : result_map ) {
        s += "\n";
        s += request;
        for ( const auto & result_code : result_codes ) {
            s += csv_separator;
            s += std::to_string( stats[ result_code ] );
        }
    }

    if ( !m_context->filename.empty() ) {
        std::ofstream out_file( m_context->filename );
        out_file << s;
    }

    if ( m_context->DUMP_TO_STDOUT ) {
        std::cout << s << std::endl;
    }

}

bool COutputProcessor::OutputStats( const bool bForceOutput ) {

    bool bResult = false;
    time_t current_time = CAggregatedStatsCollection::GetQuantizedTime( time( nullptr ) );
    bool bShouldWait = false;

    while (
        !bShouldWait
        &&
        m_context->stats.GetOldest( m_stats_item, m_stats_ts )
        &&
        (
            m_stats_ts < current_time
            ||
            bForceOutput
        )
    ) {

        m_min_unprocessed_time = CAggregatedStatsCollection::GetQuantizedTime( m_stats_ts, +1 );

        if ( time_t ts = 0; m_context->filling_line_buffers.GetOldestTimestamp( ts ) && ts < m_min_unprocessed_time ) {
            m_context->DEBUG_OUTPUT && std::cout << "Waiting for unfilled line buffer at " << ts - m_min_unprocessed_time << " seconds" << std::endl;
            bShouldWait = true;
        }

        if ( time_t ts = 0; m_context->ready_line_buffers.GetOldestTimestamp( ts ) && ts < m_min_unprocessed_time ) {
            m_context->DEBUG_OUTPUT && std::cout << "Waiting for unparsed line buffer at " << ts - m_min_unprocessed_time << " seconds" << std::endl;
            bShouldWait = true;
        }

        if ( time_t ts = 0; m_context->response_map.GetOldestTimestamp( ts ) && ts < m_min_unprocessed_time ) {
            m_context->DEBUG_OUTPUT && std::cout << "Waiting for unparsed response buffer at " << ts - m_min_unprocessed_time << " seconds" << std::endl;
            bShouldWait = true;
        }

        if ( !bShouldWait ) {
            bResult = true;
            DoOutput();
            m_context->stats.RemoveItem( m_stats_item );
        }
    }

    return bResult;
}

void COutputProcessor::Run( const std::stop_token & stoken, const PContext & context ) {
    bool bHadOutput = false;
    while ( true ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        COutputProcessor op( context );
        if ( op.OutputStats( stoken.stop_requested() && !bHadOutput ) ) {
            bHadOutput = true;
        }
        if ( stoken.stop_requested() ) {
            break;
        }
    }
}
