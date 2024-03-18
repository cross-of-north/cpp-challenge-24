#include "common.h"

#include "LineProcessor.h"

#include "MessageParser.h"

CMessageParser mp;

void CLineProcessor::ParseLineBuffer( const PLineBuffer & buffer ) {

    DEBUG_OUTPUT && std::cout << to_stream( buffer->GetTimestamp() ) << " start parsing lines" << std::endl;

    unsigned int line_count = buffer->GetCount();
    for ( unsigned int i = 0; i < line_count; i++ ) {
        mp.ProcessLine( buffer->GetItem( i ) );
        if ( mp.IsDone() ) {
            if ( mp.IsResponse() ) {
                response_map.Push( buffer->GetTimestamp(), mp.GetTraceID(), mp.GetResultCode() );
            } else {
                request_map.Push( buffer->GetTimestamp(), mp.GetTraceID(), mp.GetRequestPath() );
            }
        }
    }

    DEBUG_OUTPUT && std::cout << to_stream( buffer->GetTimestamp() ) << " end parsing lines" << std::endl;

}

void CLineProcessor::ProcessLineBuffers() {
    PLineBuffer buffer;
    while ( ready_line_buffers.GetOldestItem( buffer ) ) {
        ParseLineBuffer( buffer );
        response_map.NotifyDataAvailable();
        ready_line_buffers.RemoveItem( buffer );
    }
}

void CLineProcessor::Run( const std::stop_token & stoken ) {
    while ( true ) {
        ready_line_buffers.WaitForData( 100 );
        //std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        if ( stoken.stop_requested() && ready_line_buffers.IsEmpty() ) {
            break;
        }
        ProcessLineBuffers();
    }
}
