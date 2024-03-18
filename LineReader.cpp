#include "common.h"

#include "LineReader.h"

#include "utils.h"

void CLineReader::FlushLineBuffers() {
    PLineBuffer buffer;
    time_t buffer_ts;
    while ( filling_line_buffers.GetOldest( buffer, buffer_ts ) ) {
        ready_line_buffers.AddItem( buffer_ts, buffer );
        filling_line_buffers.RemoveItem( buffer );
    }
    ready_line_buffers.NotifyDataAvailable();
}

void CLineReader::Run( const std::stop_token & stoken ) {

    bool bPrevEmptyLine = false;
    PLineBuffer current_line_buffer;
    while ( std::cin.good() && !stoken.stop_requested() ) {
        std::string input;
        std::getline( std::cin, input );
        if (
            time_t ts = time( nullptr ) / SECONDS_PER_LINE_BUFFER;
            !current_line_buffer
            ||
            ( current_line_buffer->GetTimestamp() != ts && bPrevEmptyLine && !input.empty() )
        ) {
            if ( current_line_buffer ) {
                DEBUG_OUTPUT && std::cout << to_stream( current_line_buffer->GetTimestamp() ) << " end collecting lines" << std::endl;
            }
            FlushLineBuffers();
            current_line_buffer = std::make_shared < CLineBuffer >( ts );
            filling_line_buffers.AddItem( ts, current_line_buffer );
            DEBUG_OUTPUT && std::cout << to_stream( ts ) << " start collecting lines" << std::endl;
        }
        current_line_buffer->Push( input );
        bPrevEmptyLine = input.empty();
        //std::cout << input << std::endl;
    }
    FlushLineBuffers();

}
