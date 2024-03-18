#pragma once

#include "common.h"

#include "utils.h"

class CAggregator {

    protected:

        PEventBuffer m_buffer;
        time_t m_buffer_ts = 0;
        std::unique_ptr < std::lock_guard < std::mutex > > m_stats_item_lock;
        PAggregatedStats m_stats_item;
        time_t m_stats_item_ts = 0;

        void UseStatsItem( const time_t ts );
        void ProcessResponseBuffer();
        void ProcessResponseBuffers();

    public:

        static void Run( const std::stop_token & stoken ) ;
};
