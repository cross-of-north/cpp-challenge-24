#include "common.h"

#include "Aggregator.h"

CAggregator::CAggregator( PContext context )
    : m_context( std::move( context ) )
{
}

// removes a lock from the previously used stats item and locks new stats item as needed
void CAggregator::UseStatsItem( const time_t ts ) {
    if ( ts != m_stats_item_ts || !m_stats_item ) {
        m_context->DEBUG_OUTPUT && std::cout << to_stream( ts ) << " time of events being aggregated" << std::endl;
        m_stats_item_lock.reset();
        m_stats_item_ts = ts;
        m_context->stats.GetItemByKey( m_stats_item_ts, m_stats_item );
        m_stats_item_lock = std::make_unique < std::lock_guard < std::mutex > >( m_stats_item->GetMutex() );
    }
}

// processes a single response bucket
void CAggregator::ProcessResponseBucket() {

    m_context->DEBUG_OUTPUT && std::cout << to_stream( m_bucket_ts ) << " start aggregating events" << std::endl;

    std::string id;
    time_t result_ts = 0;
    std::string result_code;

    m_stats_item_ts = 0;
    m_stats_item.reset();

    // remove and process every event from the response bucket
    // request buckets are immutable here, old request buckets are removed fully by a cleanup thread
    while ( m_bucket->Pop( id, result_code, result_ts ) ) {
        time_t ts = 0;
        std::string request;
        UseStatsItem( CAggregatedStatsCollection::GetQuantizedTime( result_ts ) );
        auto & result_map = m_stats_item->GetStats();
        if ( m_context->request_map.GetByID( id, request, ts ) ) {
            result_map[ request ][ result_code ]++;
        } else {
            result_map[ "undefined" ][ result_code ]++;
        }
    }

    m_stats_item_lock.reset();
    m_stats_item.reset();

    m_context->DEBUG_OUTPUT && std::cout << to_stream( m_bucket_ts ) << " end aggregating events" << std::endl;

}

// processes and drops all available response buckets starting from the oldest
// (request buckets are removed by a cleanup thread)
void CAggregator::ProcessResponseBuckets() {
    while ( m_context->response_map.GetOldest( m_bucket, m_bucket_ts ) ) {
        ProcessResponseBucket();
        m_context->response_map.RemoveItem( m_bucket );
    }
}

void CAggregator::Run( const std::stop_token & stoken, const PContext & context ) {
    while ( true ) {

        // wake up every 100 ms or when new data is signaled
        context->response_map.WaitForData( 100 );

        if ( stoken.stop_requested() && context->response_map.IsEmpty() ) {
            // terminate if requested and there are no data left to process
            break;
        }

        //std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

        CAggregator aggregator( context );
        aggregator.ProcessResponseBuckets();
    }
}
