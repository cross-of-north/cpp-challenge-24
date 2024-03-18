#include "common.h"

#include "LineBuffer.h"

CLineBuffer::CLineBuffer( const time_t ts ) : m_timestamp( ts ) {
#ifdef DEBUG_MEMORY_CONSUMPTION
    std::cout << m_timestamp << " CLineBuffer()" << std::endl;
#endif // DEBUG_MEMORY_CONSUMPTION
}

CLineBuffer::~CLineBuffer() {
#ifdef DEBUG_MEMORY_CONSUMPTION
    std::cout << m_timestamp << " ~CLineBuffer()" << std::endl;
#endif // DEBUG_MEMORY_CONSUMPTION
}

void CLineBuffer::Push( const std::string & line ) {
    m_lines.push_back( line );
}

unsigned int CLineBuffer::GetCount() const {
    return m_lines.size();
}

const std::string & CLineBuffer::GetItem( const unsigned int n ) const {
    return m_lines[ n ];
}

time_t CLineBuffer::GetTimestamp() const {
    return m_timestamp;
}
