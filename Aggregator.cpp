#include "common.h"

#include "Aggregator.h"

CAggregator::CAggregator( PContext context )
    : m_context( std::move( context ) )
{
}

void CAggregator::UseStatsItem( const time_t ts ) {
    if ( ts != m_stats_item_ts || !m_stats_item ) {
        m_context->DEBUG_OUTPUT && std::cout << to_stream( ts ) << " time of events being aggregated" << std::endl;
        m_stats_item_lock.reset();
        m_stats_item_ts = ts;
        m_context->stats.GetItemByKey( m_stats_item_ts, m_stats_item );
        m_stats_item_lock = std::make_unique < std::lock_guard < std::mutex > >( m_stats_item->GetMutex() );
    }
}

void CAggregator::ProcessResponseBuffer() {

    m_context->DEBUG_OUTPUT && std::cout << to_stream( m_buffer_ts ) << " start aggregating events" << std::endl;

    std::string id;
    time_t result_ts = 0;
    std::string result_code;

    m_stats_item_ts = 0;
    m_stats_item.reset();

    while ( m_buffer->Pop( id, result_code, result_ts ) ) {
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

    m_context->DEBUG_OUTPUT && std::cout << to_stream( m_buffer_ts ) << " end aggregating events" << std::endl;

}

void CAggregator::ProcessResponseBuffers() {
    while ( m_context->response_map.GetOldest( m_buffer, m_buffer_ts ) ) {
        ProcessResponseBuffer();
        m_context->response_map.RemoveItem( m_buffer );
    }
}

void CAggregator::Run( const std::stop_token & stoken, const PContext & context ) {
    while ( true ) {
        context->response_map.WaitForData( 100 );
        if ( stoken.stop_requested() && context->response_map.IsEmpty() ) {
            break;
        }
        //std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        CAggregator aggregator( context );
        aggregator.ProcessResponseBuffers();
    }
}
