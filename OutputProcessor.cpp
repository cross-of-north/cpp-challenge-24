#include "common.h"

#include "OutputProcessor.h"

COutputProcessor::COutputProcessor( PContext context )
    : m_context( std::move( context ) )
{
}

// creates a file with the stats and prints debug data to the console
void COutputProcessor::DoOutput() {

    if ( m_context->DUMP_TO_STDOUT ) {
        std::cout << "[ " << to_stream( m_stats_ts ) << " .. " << to_stream( m_min_unprocessed_time ) << " )" << std::endl;
    }

    std::string s;

    std::lock_guard < std::mutex > lock( m_stats_item->GetMutex() );
    auto result_map = m_stats_item->GetStats();

    // collect and sort all result codes to use the column same order for the every request row
    std::set < std::string > result_codes;
    for ( const auto & stats : std::views::values( result_map ) ) {
        const auto & request_result_codes = std::views::keys( stats );
        result_codes.insert( request_result_codes.begin(), request_result_codes.end() );
    }

    const char * csv_separator = ";";//"\t";

    // request path column header
    s += "request";

    // result code headers
    for ( const auto & result_code : result_codes ) {
        s += csv_separator;
        s += result_code;
    }

    // data rows
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

// outputs ready aggregated stats as needed
bool COutputProcessor::OutputStats( const bool bForceOutput ) {

    bool bResult = false;
    bool bShouldWait = false;

    // left (inclusive) edge of an interval
    time_t current_time = CAggregatedStatsCollection::GetQuantizedTime( time( nullptr ) );

    // iterate over every stats set starting from oldest
    while (
        // while stats to output are fully ready
        !bShouldWait
        &&
        m_context->stats.GetOldest( m_stats_item, m_stats_ts )
        &&
        // and it's time to output oldest stats or output is forced (e.g. on program exit)
        (
            m_stats_ts < current_time
            ||
            bForceOutput
        )
    ) {

        // right (exclusive) edge of an interval
        m_min_unprocessed_time = CAggregatedStatsCollection::GetQuantizedTime( m_stats_ts, +1 );

        // check if there are no interval-related or older line buckets in a process of filling
        if ( time_t ts = 0; m_context->filling_line_buckets.GetOldestTimestamp( ts ) && ts < m_min_unprocessed_time ) {
            m_context->DEBUG_OUTPUT && std::cout << "Waiting for unfilled line bucket at " << ts - m_min_unprocessed_time << " seconds" << std::endl;
            bShouldWait = true;
        }

        // check if there are no interval-related or older unprocessed filled line buckets
        if ( time_t ts = 0; m_context->ready_line_buckets.GetOldestTimestamp( ts ) && ts < m_min_unprocessed_time ) {
            m_context->DEBUG_OUTPUT && std::cout << "Waiting for unparsed line bucket at " << ts - m_min_unprocessed_time << " seconds" << std::endl;
            bShouldWait = true;
        }

        // check if there are no interval-related or older unprocessed response buckets
        if ( time_t ts = 0; m_context->response_map.GetOldestTimestamp( ts ) && ts < m_min_unprocessed_time ) {
            m_context->DEBUG_OUTPUT && std::cout << "Waiting for unparsed response bucket at " << ts - m_min_unprocessed_time << " seconds" << std::endl;
            bShouldWait = true;
        }

        if ( !bShouldWait ) {
            // stats to output are fully ready
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
        bool bForceOutput = stoken.stop_requested() && !bHadOutput; // force output on exit is there was no output yet
        if ( op.OutputStats( bForceOutput ) ) {
            bHadOutput = true;
        }
        if ( stoken.stop_requested() ) {
            break;
        }
    }
}
