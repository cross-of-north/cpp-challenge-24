#pragma once

#include "common.h"

//
// Implements basic task-specific parsing of HTTP events received as a continious stream of lines with a line feed as an event end marker.
//
class CMessageParser {
    protected:
#ifdef _DEBUG
        std::string m_message;
#endif // _DEBUG
        std::string m_trace_id;
        std::string m_request_path;
        std::string m_result_code;
        bool m_bIsResponse = false;
        bool m_bDone = true;

        void Reset();
        void ProcessFirstLine( const std::string & line );
        void ProcessHeaderLine( const std::string & line );

    public:

        // process a next line of event stream
        void ProcessLine( const std::string & line );

        // returns true if event is fully processed (LF was received)
        bool IsDone() const;

        // returns true if a current event is a response (false if it is a request)
        bool IsResponse() const;

        // returns a request path of a current event (if it is a request)
        const std::string & GetRequestPath() const;

        // returns an X-Trace-ID of a current event
        const std::string & GetTraceID() const;

        // returns a result code of a current event (if it is a response)
        const std::string & GetResultCode() const;
};
