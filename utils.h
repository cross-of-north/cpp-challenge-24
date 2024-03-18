#pragma once

#include "common.h"
#include "LineBuffer.h"
#include "EventBuffer.h"
#include "AggregatedStats.h"

constexpr time_t SECONDS_PER_LINE_BUFFER = 1;
constexpr time_t SECONDS_PER_EVENT_BUFFER = 1;
constexpr time_t SECONDS_PER_OUTPUT = 60;
constexpr time_t REQUEST_LIFETIME_IN_SECONDS = 20;

//#define DEBUG_MEMORY_CONSUMPTION 1

inline auto to_stream( const time_t tp ) {
    return std::put_time( std::localtime( &tp ), "%F %T %Z" );
}

class CContext {

    public:

        bool DEBUG_OUTPUT = false;
        bool DUMP_TO_STDOUT = true;
        std::string filename;

        CEventBuffers request_map;
        CEventBuffers response_map;
        CLineBuffers filling_line_buffers;
        CLineBuffers ready_line_buffers;
        CAggregatedStatsCollection stats;
};
typedef std::shared_ptr < CContext > PContext;
