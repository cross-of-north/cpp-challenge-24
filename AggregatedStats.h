#pragma once

#include "common.h"
#include "TimeKeyedCollection.h"

//
// One set of aggregated stats
//
class CAggregatedStats {

    public:

        typedef std::map < std::string, std::string > t_string_to_string_map;
        typedef std::map < std::string, long long unsigned int > t_string_to_uint64_map;
        typedef std::map < std::string, t_string_to_uint64_map > t_aggregated_stats;

    protected:

        std::mutex m_mutex;
        t_aggregated_stats m_stats;

    public:

        // mutex to lock this object
        std::mutex & GetMutex();

        // immediate stats data r/w access
        CAggregatedStats::t_aggregated_stats & GetStats();
};
typedef std::shared_ptr < CAggregatedStats > PAggregatedStats;

//
// Collection of aggregated stats keyed by interval start time
//
class CAggregatedStatsCollection : public CTimeKeyedCollection < CAggregatedStats > {

    public:

        // returns timestamp rounded down to the nearest interval start time or to the nearest plus delta interval start time
        static time_t GetQuantizedTime( const time_t ts, const time_t delta = 0 );

};
