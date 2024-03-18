#include "common.h"

#include "utils.h"

#include "LineReader.h"
#include "LineProcessor.h"
#include "Aggregator.h"
#include "OutputProcessor.h"

void Cleanup( const std::stop_token & stoken, const PContext & context ) {
    while ( !stoken.stop_requested() ) {
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        context->request_map.DiscardOlderThan( time( nullptr ) - REQUEST_LIFETIME_IN_SECONDS );
    }
}

int main( const int argc, const char **argv ) {

    PContext context( std::make_shared<CContext>() );

    if ( argc > 1 ) {
        if ( argc == 3 && argv[ 1 ] == std::string( "-o" ) ) {
            //context->DUMP_TO_STDOUT = false;
            context->filename = argv[ 2 ];
        } else {
            std::cout << "Usage: " << argv[ 0 ] << " [-o <output file>]" << std::endl;
            return -1;
        }
    }

    // pipeline:
    //
    // LineReader -> (CLineBuffers ready_line_buffers) ->
    //  -> LineProcessor -> (CEventBuffers request_map, response_map) ->
    //  -> Aggregator -> (CAggregatedStatsCollection) ->
    //  -> OutputProcessor -> (file)
    //

    auto read_thread = std::jthread( CLineReader::Run, context );
    auto cleanup_thread = std::jthread( Cleanup, context );
    auto parse_thread = std::jthread( CLineProcessor::Run, context );
    auto aggregate_thread = std::jthread( CAggregator::Run, context );
    auto output_thread = std::jthread( COutputProcessor::Run, context );

    read_thread.join();

    cleanup_thread.request_stop();
    cleanup_thread.join();
    parse_thread.request_stop();
    parse_thread.join();
    aggregate_thread.request_stop();
    aggregate_thread.join();
    output_thread.request_stop();
    output_thread.join();

    return 0;
}
