#pragma once

#include "common.h"

#include "utils.h"

//
// Implements aggregation of request/response events received with a set time granularity, drops processed response buckets
//
class CAggregator {

    protected:

        PEventBucket m_bucket;
        time_t m_bucket_ts = 0;
        std::unique_ptr < std::lock_guard < std::mutex > > m_stats_item_lock;
        PAggregatedStats m_stats_item;
        time_t m_stats_item_ts = 0;
        PContext m_context;

        void UseStatsItem( const time_t ts );
        void ProcessResponseBucket();
        void ProcessResponseBuckets();

        explicit CAggregator( PContext context );

    public:

        static void Run( const std::stop_token & stoken, const PContext & context ) ;
};
