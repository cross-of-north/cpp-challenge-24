#include "common.h"

#include "EventBucket.h"

#include "utils.h"

CEventBucket::CEventBucket() {
#ifdef DEBUG_MEMORY_CONSUMPTION
    std::cout << "CEventBucket()" << std::endl;
#endif // DEBUG_MEMORY_CONSUMPTION
}

CEventBucket::~CEventBucket() {
#ifdef DEBUG_MEMORY_CONSUMPTION
    std::cout << "~CEventBucket()" << std::endl;
#endif // DEBUG_MEMORY_CONSUMPTION
}

void CEventBucket::Push( const time_t ts, const std::string & id, const std::string & s ) {
    std::unique_lock < std::shared_mutex > lock( m_mutex );
    m_events.emplace( std::make_pair( id, std::make_tuple( ts, s ) ) );
}

bool CEventBucket::GetByID( const std::string & id, std::string & value, time_t & ts ) const {
    bool bResult = false;
    value.clear();
    ts = 0;
    std::shared_lock < std::shared_mutex > lock( m_mutex );
    const auto & it = m_events.find( id );
    if ( it != m_events.end() ) {
        ts = std::get< 0 >( it->second );
        value = std::get< 1 >( it->second );
        bResult = true;
    }
    return bResult;
}

bool CEventBucket::Pop( std::string & id, std::string & value, time_t & ts ) {
    bool bResult = false;
    id.clear();
    value.clear();
    ts = 0;
    std::unique_lock < std::shared_mutex > lock( m_mutex );
    const auto & it = m_events.begin();
    if ( it != m_events.end() ) {
        id = it->first;
        ts = std::get< 0 >( it->second );
        value = std::get< 1 >( it->second );
        m_events.erase( it );
        bResult = true;
    }
    return bResult;
}

void CEventBuckets::Push( const time_t ts, const std::string & id, const std::string & s ) {
    time_t time_key = ts / SECONDS_PER_EVENT_BUCKET;
    PEventBucket event_bucket;
    GetItemByKey( time_key, event_bucket );
    if ( event_bucket ) {
        event_bucket->Push( ts, id, s );
    } else {
        // assert( false );
    }
}

bool CEventBuckets::GetByID( const std::string & id, std::string & value, time_t & ts ) const {
    bool bResult = false;
    value.clear();
    ts = 0;
    std::shared_lock < std::shared_mutex > lock( m_mutex );
    // buckets are checked in reverse iterator order (starting from the newest) because the probability
    // of finding a request in recent request buckets is higher (optimistically assuming that the request is processed quickly)
    for ( const auto & [ bucket_timestamp, bucket ] : std::ranges::reverse_view( m_container ) ) {
        if ( bucket->GetByID( id, value, ts ) ) {
            bResult = true;
            break;
        }
    }
    return bResult;
}
