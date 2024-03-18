#include "common.h"

#include "EventBuffer.h"

#include "utils.h"

CEventBuffer::CEventBuffer() {
#ifdef DEBUG_MEMORY_CONSUMPTION
    std::cout << "CEventBuffer()" << std::endl;
#endif // DEBUG_MEMORY_CONSUMPTION
}

CEventBuffer::~CEventBuffer() {
#ifdef DEBUG_MEMORY_CONSUMPTION
    std::cout << "~CEventBuffer()" << std::endl;
#endif // DEBUG_MEMORY_CONSUMPTION
}

void CEventBuffer::Push( const time_t ts, const std::string & id, const std::string & s ) {
    std::unique_lock < std::shared_mutex > lock( m_mutex );
    m_events.emplace( std::make_pair( id, std::make_tuple( ts, s ) ) );
}

bool CEventBuffer::GetByID( const std::string & id, std::string & value, time_t & ts ) const {
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

bool CEventBuffer::Pop( std::string & id, std::string & value, time_t & ts ) {
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

void CEventBuffers::Push( const time_t ts, const std::string & id, const std::string & s ) {
    time_t time_key = ts / SECONDS_PER_EVENT_BUFFER;
    PEventBuffer event_buffer;
    GetItemByKey( time_key, event_buffer );
    if ( event_buffer ) {
        event_buffer->Push( ts, id, s );
    } else {
        // assert( false );
    }
}

bool CEventBuffers::GetByID( const std::string & id, std::string & value, time_t & ts ) const {
    bool bResult = false;
    value.clear();
    ts = 0;
    std::shared_lock < std::shared_mutex > lock( m_mutex );
    for ( const auto & [ buffer_timestamp, buffer ] : std::ranges::reverse_view( m_container ) ) {
        if ( buffer->GetByID( id, value, ts ) ) {
            bResult = true;
            break;
        }
    }
    return bResult;
}
