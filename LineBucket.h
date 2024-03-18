#pragma once

#include "TimeKeyedCollection.h"

//
// A bucket of unprocessed lines received from the input stream in a specified epoch second.
// Interface is separated from the implementation (std::vector) to allow for easy replacement of the underlying storage/line supplier.
//
class CLineBucket {

    private:

        typedef std::vector < std::string > t_lines;
        t_lines m_lines;
        time_t m_timestamp = 0;

    public:

        CLineBucket() = delete;
        explicit CLineBucket( const time_t ts );
        ~CLineBucket();

        // stores a line
        void Push( const std::string & line );

        // returns a count of lines
        unsigned int GetCount() const;

        // returns a line by index
        const std::string & GetItem( const unsigned int n ) const;

        // returns the timestamp of the bucket
        time_t GetTimestamp() const;
};

typedef std::shared_ptr < CLineBucket > PLineBucket;

//
// Collection of line buckets keyed by time of receival (epoch second)
//
class CLineBuckets : public CTimeKeyedCollection < CLineBucket > {
};
