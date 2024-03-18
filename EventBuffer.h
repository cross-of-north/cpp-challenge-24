#pragma once

#include "common.h"
#include "TimeKeyedCollection.h"

class CEventBuffer {

    private:

        typedef std::tuple < time_t, std::string > t_event;
        typedef std::map < std::string, t_event > t_event_map;

        t_event_map m_events;
        mutable std::shared_mutex m_mutex;

    public:

        CEventBuffer();
        ~CEventBuffer();

        void Push( const time_t ts, const std::string & id, const std::string & s );
        bool GetByID( const std::string & id, std::string & value, time_t & ts ) const;
        bool Pop( std::string & id, std::string & value, time_t & ts );

};
typedef std::shared_ptr < CEventBuffer > PEventBuffer;

class CEventBuffers : public CTimeKeyedCollection < CEventBuffer > {

    public:

        void Push( const time_t ts, const std::string & id, const std::string & s );
        bool GetByID( const std::string & id, std::string & value, time_t & ts ) const;

};
