#include "common.h"

#include "LineProcessor.h"

#include "MessageParser.h"

CMessageParser mp;

CLineProcessor::CLineProcessor( PContext context )
    : m_context( std::move( context ) )
{
}

void CLineProcessor::ParseLineBuffer( const PLineBuffer & buffer ) {

    m_context->DEBUG_OUTPUT && std::cout << to_stream( buffer->GetTimestamp() ) << " start parsing lines" << std::endl;

    unsigned int line_count = buffer->GetCount();
    for ( unsigned int i = 0; i < line_count; i++ ) {
        mp.ProcessLine( buffer->GetItem( i ) );
        if ( mp.IsDone() ) {
            if ( mp.IsResponse() ) {
                m_context->response_map.Push( buffer->GetTimestamp(), mp.GetTraceID(), mp.GetResultCode() );
            } else {
                m_context->request_map.Push( buffer->GetTimestamp(), mp.GetTraceID(), mp.GetRequestPath() );
            }
        }
    }

    m_context->DEBUG_OUTPUT && std::cout << to_stream( buffer->GetTimestamp() ) << " end parsing lines" << std::endl;

}

void CLineProcessor::ProcessLineBuffers() {
    PLineBuffer buffer;
    while ( m_context->ready_line_buffers.GetOldestItem( buffer ) ) {
        ParseLineBuffer( buffer );
        m_context->response_map.NotifyDataAvailable();
        m_context->ready_line_buffers.RemoveItem( buffer );
    }
}

void CLineProcessor::Run( const std::stop_token & stoken, const PContext & context ) {
    while ( true ) {
        context->ready_line_buffers.WaitForData( 100 );
        //std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        if ( stoken.stop_requested() && context->ready_line_buffers.IsEmpty() ) {
            break;
        }
        CLineProcessor lp( context );
        lp.ProcessLineBuffers();
    }
}
