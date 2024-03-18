#include "common.h"

#include "LineReader.h"

#include "utils.h"

CLineReader::CLineReader( PContext context )
    : m_context( std::move( context ) )
{
}

// moves all (a single existing) filling line buckets to a filled (ready) line buckets collection
void CLineReader::FlushLineBuckets() const {
    PLineBucket bucket;
    time_t bucket_ts;
    while ( m_context->filling_line_buckets.GetOldest( bucket, bucket_ts ) ) {
        m_context->ready_line_buckets.AddItem( bucket_ts, bucket );
        m_context->filling_line_buckets.RemoveItem( bucket );
    }
    m_context->ready_line_buckets.NotifyDataAvailable();
}

// processes one line of input
void CLineReader::ReadLine() {

    std::string input;
    std::getline( std::cin, input );
    if (
        time_t ts = time( nullptr ) / SECONDS_PER_LINE_BUCKET;
        // switch buckets if there is no current bucket
        !m_current_line_bucket
        ||
        // or if the current timestamp is different from the current bucket's timestamp
        // and the next event start is reached (non-empty line after empty line).
        ( m_current_line_bucket->GetTimestamp() != ts && m_bPrevEmptyLine && !input.empty() )
    ) {

        if ( m_current_line_bucket ) {
            m_context->DEBUG_OUTPUT && std::cout << to_stream( m_current_line_bucket->GetTimestamp() ) << " end collecting lines" << std::endl;
        }

        // submit data collected for processing
        FlushLineBuckets();

        // Lines are marked by a current timestamp (time() value).
        // "Date:" header is ignored because its source is unknown (not trusted), so using its value can distort aggregation results.
        m_current_line_bucket = std::make_shared < CLineBucket >( ts );

        m_context->filling_line_buckets.AddItem( ts, m_current_line_bucket );
        m_context->DEBUG_OUTPUT && std::cout << to_stream( ts ) << " start collecting lines" << std::endl;

    }

    m_current_line_bucket->Push( input );
    m_bPrevEmptyLine = input.empty();
    //std::cout << input << std::endl;

}

void CLineReader::Run( const std::stop_token & stoken, const PContext & context ) {

    CLineReader lr( context );
    while ( std::cin.good() && !stoken.stop_requested() ) {
        lr.ReadLine();
    }

    // submit remaining data collected for processing
    lr.FlushLineBuckets();

}
