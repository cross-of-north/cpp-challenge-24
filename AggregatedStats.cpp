#include "common.h"

#include "AggregatedStats.h"

#include "utils.h"

std::mutex & CAggregatedStats::GetMutex() {
    return m_mutex;
}

CAggregatedStats::t_aggregated_stats & CAggregatedStats::GetStats() {
    return m_stats;
}

time_t CAggregatedStatsCollection::GetQuantizedTime( const time_t ts, const time_t delta ) {
    return ( ts / SECONDS_PER_OUTPUT + delta ) * SECONDS_PER_OUTPUT;
}
