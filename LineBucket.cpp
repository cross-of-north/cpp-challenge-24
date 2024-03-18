#include "common.h"

#include "LineBucket.h"

CLineBucket::CLineBucket( const time_t ts ) : m_timestamp( ts ) {
#ifdef DEBUG_MEMORY_CONSUMPTION
    std::cout << m_timestamp << " CLineBucket()" << std::endl;
#endif // DEBUG_MEMORY_CONSUMPTION
}

CLineBucket::~CLineBucket() {
#ifdef DEBUG_MEMORY_CONSUMPTION
    std::cout << m_timestamp << " ~CLineBucket()" << std::endl;
#endif // DEBUG_MEMORY_CONSUMPTION
}

void CLineBucket::Push( const std::string & line ) {
    m_lines.push_back( line );
}

unsigned int CLineBucket::GetCount() const {
    return m_lines.size();
}

const std::string & CLineBucket::GetItem( const unsigned int n ) const {
    return m_lines[ n ];
}

time_t CLineBucket::GetTimestamp() const {
    return m_timestamp;
}
