#include "common.h"

#include "LineReader.h"

#include "utils.h"

CLineReader::CLineReader( PContext context )
    : m_context( std::move( context ) )
{
}

void CLineReader::FlushLineBuffers() const {
    PLineBuffer buffer;
    time_t buffer_ts;
    while ( m_context->filling_line_buffers.GetOldest( buffer, buffer_ts ) ) {
        m_context->ready_line_buffers.AddItem( buffer_ts, buffer );
        m_context->filling_line_buffers.RemoveItem( buffer );
    }
    m_context->ready_line_buffers.NotifyDataAvailable();
}

void CLineReader::ReadLine() {

    std::string input;
    std::getline( std::cin, input );
    if (
        time_t ts = time( nullptr ) / SECONDS_PER_LINE_BUFFER;
        !m_current_line_buffer
        ||
        ( m_current_line_buffer->GetTimestamp() != ts && m_bPrevEmptyLine && !input.empty() )
    ) {
        if ( m_current_line_buffer ) {
            m_context->DEBUG_OUTPUT && std::cout << to_stream( m_current_line_buffer->GetTimestamp() ) << " end collecting lines" << std::endl;
        }
        FlushLineBuffers();
        m_current_line_buffer = std::make_shared < CLineBuffer >( ts );
        m_context->filling_line_buffers.AddItem( ts, m_current_line_buffer );
        m_context->DEBUG_OUTPUT && std::cout << to_stream( ts ) << " start collecting lines" << std::endl;
    }
    m_current_line_buffer->Push( input );
    m_bPrevEmptyLine = input.empty();
    //std::cout << input << std::endl;

}

void CLineReader::Run( const std::stop_token & stoken, const PContext & context ) {

    CLineReader lr( context );
    while ( std::cin.good() && !stoken.stop_requested() ) {
        lr.ReadLine();
    }
    lr.FlushLineBuffers();

}
