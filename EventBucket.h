#pragma once

#include "common.h"
#include "TimeKeyedCollection.h"

//
// A bucket of events of the same type (requests or responses) keyed by X-Trace-ID.
// Event is represented by a timestamp and one string.
//
class CEventBucket {

    private:

        typedef std::tuple < time_t, std::string > t_event;
        typedef std::map < std::string, t_event > t_event_map;

        t_event_map m_events;
        mutable std::shared_mutex m_mutex;

    public:

        CEventBucket();
        ~CEventBucket();

        // adds event to the bucket
        void Push( const time_t ts, const std::string & id, const std::string & s );

        // finds event by X-Trace-ID, returns false if not found
        bool GetByID( const std::string & id, std::string & value, time_t & ts ) const;

        // removes and returns the oldest event from the bucket, returns false if the bucket is empty
        bool Pop( std::string & id, std::string & value, time_t & ts );

};
typedef std::shared_ptr < CEventBucket > PEventBucket;

//
// Collection of event buckets keyed by event start time (epoch second)
//
class CEventBuckets : public CTimeKeyedCollection < CEventBucket > {

    public:

        // adds event to the corresponding bucket
        void Push( const time_t ts, const std::string & id, const std::string & s );

        // finds event by X-Trace-ID starting from the newest bucket, returns false if not found
        bool GetByID( const std::string & id, std::string & value, time_t & ts ) const;

};
