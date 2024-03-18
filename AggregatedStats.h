#pragma once

#include "common.h"
#include "TimeKeyedCollection.h"

class CAggregatedStats {
    public:
        typedef std::map < std::string, std::string > t_string_to_string_map;
        typedef std::map < std::string, long long unsigned int > t_string_to_uint64_map;
        typedef std::map < std::string, t_string_to_uint64_map > t_aggregated_stats;
    protected:
        std::mutex m_mutex;
        t_aggregated_stats m_stats;
    public:

        std::mutex & GetMutex();

        CAggregatedStats::t_aggregated_stats & GetStats();
};
typedef std::shared_ptr < CAggregatedStats > PAggregatedStats;

class CAggregatedStatsCollection : public CTimeKeyedCollection < CAggregatedStats > {
    public:
        static time_t GetQuantizedTime( const time_t ts, const time_t delta = 0 );
};
