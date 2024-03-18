#pragma once

#include "common.h"

class CMessageProcessor {
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
        void ProcessLine( const std::string & line );

        bool IsDone() const;
        bool IsResponse() const;
        const std::string & GetRequestPath() const;
        const std::string & GetTraceID() const;
        const std::string & GetResultCode() const;
};
