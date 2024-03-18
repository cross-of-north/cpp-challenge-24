#include "common.h"

#include "utils.h"

#include "LineReader.h"
#include "LineProcessor.h"
#include "Aggregator.h"
#include "OutputProcessor.h"

//
// Cleans up old request buckets from the request_map
//
void Cleanup( const std::stop_token & stoken, const PContext & context ) {
    while ( !stoken.stop_requested() ) {
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        context->request_map.DiscardOlderThan( time( nullptr ) - REQUEST_LIFETIME_IN_SECONDS );
    }
}

int main( const int argc, const char **argv ) {

    // common variables which should be accessible from all threads
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

    //
    // Data pipeline:
    //
    // LineReader -> (CLineBuckets ready_line_buckets) ->
    //  -> LineProcessor -> (CEventBuckets request_map, response_map) ->
    //  -> Aggregator -> (CAggregatedStatsCollection) ->
    //  -> OutputProcessor -> (file)
    //

    auto read_thread = std::jthread( CLineReader::Run, context );
    auto cleanup_thread = std::jthread( Cleanup, context );
    auto parse_thread = std::jthread( CLineProcessor::Run, context );
    auto aggregate_thread = std::jthread( CAggregator::Run, context );
    auto output_thread = std::jthread( COutputProcessor::Run, context );

    read_thread.join();  // this will block until the STDIN pipe or file is closed

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
