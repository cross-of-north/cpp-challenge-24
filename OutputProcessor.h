#pragma once

#include "common.h"
#include "utils.h"

class COutputProcessor {

    protected:

        time_t m_stats_ts = 0;
        time_t m_min_unprocessed_time = 0;
        PAggregatedStats m_stats_item;
        PContext m_context;

        void DoOutput();
        bool OutputStats( const bool bForceOutput );

        explicit COutputProcessor( PContext context );

    public:

        static void Run( const std::stop_token & stoken, const PContext & context );

};
