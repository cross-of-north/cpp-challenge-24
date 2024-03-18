#include "common.h"

#include "MessageProcessor.h"

void CMessageProcessor::Reset() {
#ifdef _DEBUG
    m_message.clear();
#endif // _DEBUG
    m_trace_id.clear();
    m_request_path.clear();
    m_result_code.clear();
    m_bIsResponse = false;
    m_bDone = false;
}

void CMessageProcessor::ProcessFirstLine( const std::string & line ) {
    m_bIsResponse = ( line.rfind( "HTTP/", 0 ) == 0 );
    auto second_token_start = line.find( ' ' );
    if ( second_token_start != std::string::npos ) {
        auto third_token_start = line.find( ' ', second_token_start + 1 );
        if ( third_token_start == std::string::npos ) {
            third_token_start = line.length();
        }
        auto second_token = line.substr( second_token_start + 1, third_token_start - second_token_start - 1 );
        if ( m_bIsResponse ) {
            m_result_code = second_token;
        } else {
            m_request_path = second_token;
        }
    }
}

void CMessageProcessor::ProcessHeaderLine( const std::string & line ) {
    static const std::string trace_id_prefix( "X-Trace-ID: " );
    if ( line.rfind( trace_id_prefix, 0 ) == 0 ) {
        m_trace_id = line.substr( trace_id_prefix.length() );
    }
}

void CMessageProcessor::ProcessLine( const std::string & line ) {
    if ( line.empty() ) {
        m_bDone = true;
    } else {
        if ( m_bDone ) {
            Reset();
            ProcessFirstLine( line );
        } else {
            ProcessHeaderLine( line );
        }
#ifdef _DEBUG
        m_message += line;
        m_message += '\n';
#endif // _DEBUG
    }
}

bool CMessageProcessor::IsDone() const {
    return m_bDone;
}

bool CMessageProcessor::IsResponse() const {
    return m_bIsResponse;
}

const std::string & CMessageProcessor::GetRequestPath() const {
    return m_request_path;
}

const std::string & CMessageProcessor::GetTraceID() const {
    return m_trace_id;
}

const std::string & CMessageProcessor::GetResultCode() const {
    return m_result_code;
}
