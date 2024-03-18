#pragma once

#include "common.h"
#include "LineBuffer.h"
#include "EventBuffer.h"
#include "AggregatedStats.h"

constexpr time_t SECONDS_PER_LINE_BUFFER = 1;
constexpr time_t SECONDS_PER_EVENT_BUFFER = 1;
constexpr time_t SECONDS_PER_OUTPUT = 5;
constexpr time_t REQUEST_LIFETIME_IN_SECONDS = 20;

//#define DEBUG_MEMORY_CONSUMPTION 1
extern bool DEBUG_OUTPUT;

extern bool DUMP_TO_STDOUT;
extern std::string filename;

inline auto to_stream( const time_t tp ) {
    return std::put_time( std::localtime( &tp ), "%F %T %Z" );
}

extern CEventBuffers request_map;
extern CEventBuffers response_map;
extern CLineBuffers filling_line_buffers;
extern CLineBuffers ready_line_buffers;
extern CAggregatedStatsCollection stats;
