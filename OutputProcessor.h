#pragma once

#include "common.h"
#include "utils.h"

class COutputProcessor {

    protected:

        time_t m_stats_ts = 0;
        time_t m_min_unprocessed_time = 0;
        PAggregatedStats m_stats_item;
        void DoOutput();
        bool OutputStats( const bool bForceOutput );

    public:

        static void Run( const std::stop_token & stoken );

};
