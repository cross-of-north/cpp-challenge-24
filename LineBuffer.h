#pragma once

#include "TimeKeyedCollection.h"

class CLineBuffer {

    private:

        typedef std::vector < std::string > t_lines;
        t_lines m_lines;
        time_t m_timestamp = 0;

    public:

        CLineBuffer() = delete;
        explicit CLineBuffer( const time_t ts );
        ~CLineBuffer();

        void Push( const std::string & line );
        unsigned int GetCount() const;
        const std::string & GetItem( const unsigned int n ) const;
        time_t GetTimestamp() const;
};

typedef std::shared_ptr < CLineBuffer > PLineBuffer;

class CLineBuffers : public CTimeKeyedCollection < CLineBuffer > {
};
