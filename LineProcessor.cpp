#include "common.h"

#include "LineProcessor.h"

#include "MessageParser.h"

CMessageParser mp;

CLineProcessor::CLineProcessor( PContext context )
    : m_context( std::move( context ) )
{
}

// processes one line bucket
void CLineProcessor::ParseLineBucket( const PLineBucket & bucket ) {

    m_context->DEBUG_OUTPUT && std::cout << to_stream( bucket->GetTimestamp() ) << " start parsing lines" << std::endl;

    unsigned int line_count = bucket->GetCount();
    for ( unsigned int i = 0; i < line_count; i++ ) {
        mp.ProcessLine( bucket->GetItem( i ) );
        if ( mp.IsDone() ) {
            if ( mp.IsResponse() ) {
                m_context->response_map.Push( bucket->GetTimestamp(), mp.GetTraceID(), mp.GetResultCode() );
            } else {
                m_context->request_map.Push( bucket->GetTimestamp(), mp.GetTraceID(), mp.GetRequestPath() );
            }
        }
    }

    m_context->DEBUG_OUTPUT && std::cout << to_stream( bucket->GetTimestamp() ) << " end parsing lines" << std::endl;

}

// processes all collected line buckets
void CLineProcessor::ProcessLineBuckets() {
    PLineBucket bucket;
    while ( m_context->ready_line_buckets.GetOldestItem( bucket ) ) {
        ParseLineBucket( bucket );
        m_context->response_map.NotifyDataAvailable();
        m_context->ready_line_buckets.RemoveItem( bucket );
    }
}

void CLineProcessor::Run( const std::stop_token & stoken, const PContext & context ) {
    while ( true ) {
        context->ready_line_buckets.WaitForData( 100 );
        //std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        if ( stoken.stop_requested() && context->ready_line_buckets.IsEmpty() ) {
            // terminate if requested and there are no data left to process
            break;
        }
        CLineProcessor lp( context );
        lp.ProcessLineBuckets();
    }
}
