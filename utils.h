#pragma once

#include "common.h"
#include "LineBucket.h"
#include "EventBucket.h"
#include "AggregatedStats.h"

// values other than 1 are not tested
constexpr time_t SECONDS_PER_LINE_BUCKET = 1;
constexpr time_t SECONDS_PER_EVENT_BUCKET = 1;

// width of the aggregated time interval
constexpr time_t SECONDS_PER_OUTPUT = 60;

// maximum time to wait for the response to arrive before dropping request data
constexpr time_t REQUEST_LIFETIME_IN_SECONDS = 20;

//#define DEBUG_MEMORY_CONSUMPTION 1

inline auto to_stream( const time_t tp ) {
    return std::put_time( std::localtime( &tp ), "%F %T %Z" );
}

//
// Data accessed from different threads.
// Thread-safety is provided separately for every field.
//
class CContext {

    public:

        bool DEBUG_OUTPUT = false;
        bool DUMP_TO_STDOUT = true;
        std::string filename;

        CEventBuckets request_map;
        CEventBuckets response_map;
        CLineBuckets filling_line_buckets;
        CLineBuckets ready_line_buckets;
        CAggregatedStatsCollection stats;
};
typedef std::shared_ptr < CContext > PContext;
