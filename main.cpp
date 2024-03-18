#include "common.h"

#include "utils.h"
#include "MessageProcessor.h"

CMessageProcessor mp;

void OutputStats( const std::stop_token & stoken ) {

    bool bHadOutput = false;

    while ( true ) {

        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        time_t current_time = CAggregatedStatsCollection::GetQuantizedTime( time( nullptr ) );

        bool bShouldWait = false;

        PAggregatedStats stats_item;
        time_t stats_ts = 0;
        while (
            !bShouldWait
            &&
            (
                ( stats.GetOldest( stats_item, stats_ts ) && stats_ts < current_time )
                ||
                ( stoken.stop_requested() && !bHadOutput )
            )
        ) {

            time_t min_unprocessed_time = CAggregatedStatsCollection::GetQuantizedTime( stats_ts, +1 );

            if ( time_t ts = 0; filling_line_buffers.GetOldestTimestamp( ts ) && ts < min_unprocessed_time ) {
                DEBUG_OUTPUT && std::cout << "Waiting for unfilled line buffer at " << ts - min_unprocessed_time << " seconds" << std::endl;
                bShouldWait = true;
            }

            if ( time_t ts = 0; ready_line_buffers.GetOldestTimestamp( ts ) && ts < min_unprocessed_time ) {
                DEBUG_OUTPUT && std::cout << "Waiting for unparsed line buffer at " << ts - min_unprocessed_time << " seconds" << std::endl;
                bShouldWait = true;
            }

            if ( time_t ts = 0; response_map.GetOldestTimestamp( ts ) && ts < min_unprocessed_time ) {
                DEBUG_OUTPUT && std::cout << "Waiting for unparsed response buffer at " << ts - min_unprocessed_time << " seconds" << std::endl;
                bShouldWait = true;
            }

            if ( !bShouldWait ) {

                bHadOutput = true;

                std::ostream * pout = &std::cout;
                std::unique_ptr < std::ofstream > pout_file;
                if ( !DUMP_TO_STDOUT ) {
                    pout_file = std::make_unique < std::ofstream >(
                        filename.empty()
                        ?
                        std::to_string( stats_ts ) + "-" + std::to_string( min_unprocessed_time ) + ".csv"
                        :
                        filename
                    );
                    pout = pout_file.get();
                }

                if ( DUMP_TO_STDOUT ) {
                    *pout << "[ " << to_stream( stats_ts ) << " .. " << to_stream( min_unprocessed_time ) << " )" << std::endl;
                }

                std::lock_guard < std::mutex > lock( stats_item->GetMutex() );
                auto result_map = stats_item->GetStats();

                std::set < std::string > result_codes;
                for ( const auto & stats : std::views::values( result_map ) ) {
                    const auto & request_result_codes = std::views::keys( stats );
                    result_codes.insert( request_result_codes.begin(), request_result_codes.end() );
                }

                const char * csv_separator = ";";//"\t";

                *pout << "request";
                for ( const auto & result_code : result_codes ) {
                    *pout << csv_separator << result_code;
                }
                for ( auto & [ request, stats ] : result_map ) {
                    *pout << std::endl;
                    *pout << request;
                    for ( const auto & result_code : result_codes ) {
                        *pout << csv_separator << stats[ result_code ];
                    }
                }

                *pout << std::endl;

                stats.RemoveItem( stats_item );
            }
        }

        if ( stoken.stop_requested() ) {
            break;
        }

    }
}

void Aggregate( const std::stop_token & stoken ) {

    while ( true ) {

        response_map.WaitForData( 100 );

        if ( stoken.stop_requested() && response_map.IsEmpty() ) {
            break;
        }

        //std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

        PEventBuffer buffer;
        time_t buffer_ts;
        while ( response_map.GetOldest( buffer, buffer_ts ) ) {
            DEBUG_OUTPUT && std::cout << to_stream( buffer_ts ) << " start aggregating events" << std::endl;
            {
                std::string id;
                time_t result_ts = 0;
                std::string result_code;

                std::unique_ptr < std::lock_guard < std::mutex > > lock;
                PAggregatedStats stats_item;
                time_t stats_time = 0;

                while ( buffer->Pop( id, result_code, result_ts ) ) {
                    time_t ts = 0;
                    std::string request;
                    time_t current_stats_time = CAggregatedStatsCollection::GetQuantizedTime( result_ts );
                    if ( current_stats_time != stats_time || !stats_item ) {
                        DEBUG_OUTPUT && std::cout << to_stream( current_stats_time ) << " time of events being aggregated" << std::endl;
                        lock.reset();
                        stats_time = current_stats_time;
                        stats.GetItemByKey( stats_time, stats_item );
                        lock = std::make_unique < std::lock_guard < std::mutex > >( stats_item->GetMutex() );
                    }
                    auto & result_map = stats_item->GetStats();
                    if ( request_map.GetByID( id, request, ts ) ) {
                        result_map[ request ][ result_code ]++;
                    } else {
                        result_map[ "undefined" ][ result_code ]++;
                    }
                }
            }
            DEBUG_OUTPUT && std::cout << to_stream( buffer_ts ) << " end aggregating events" << std::endl;
            response_map.RemoveItem( buffer );
        }

    }

}

void ParseLineBuffer( const PLineBuffer & buffer ) {

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

void ProcessLineBuffers( const std::stop_token & stoken ) {

    while ( true ) {

        ready_line_buffers.WaitForData( 100 );

        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

        if ( stoken.stop_requested() && ready_line_buffers.IsEmpty() ) {
            break;
        }

        PLineBuffer buffer;

        while ( ready_line_buffers.GetOldestItem( buffer ) ) {
            ParseLineBuffer( buffer );
            response_map.NotifyDataAvailable();
            ready_line_buffers.RemoveItem( buffer );
        }
    }
}

void FlushLineBuffers() {
    PLineBuffer buffer;
    time_t buffer_ts;
    while ( filling_line_buffers.GetOldest( buffer, buffer_ts ) ) {
        ready_line_buffers.AddItem( buffer_ts, buffer );
        filling_line_buffers.RemoveItem( buffer );
    }
    ready_line_buffers.NotifyDataAvailable();
}

void ReadSTDIN( const std::stop_token & stoken ) {

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

void Cleanup( const std::stop_token & stoken ) {
    while ( !stoken.stop_requested() ) {
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        request_map.DiscardOlderThan( time( nullptr ) - REQUEST_LIFETIME_IN_SECONDS );
    }
}

int main( const int argc, const char **argv ) {

    DEBUG_OUTPUT = false;

    if ( argc > 1 ) {
        if ( argc == 3 && argv[ 1 ] == std::string( "-o" ) ) {
            DUMP_TO_STDOUT = false;
            filename = argv[ 2 ];
        } else {
            std::cout << "Usage: " << argv[ 0 ] << " [-o <output file>]" << std::endl;
            return -1;
        }
    }

    auto read_thread = std::jthread( ReadSTDIN );
    auto cleanup_thread = std::jthread( Cleanup );
    auto parse_thread = std::jthread( ProcessLineBuffers );
    auto aggregate_thread = std::jthread( Aggregate );
    auto output_thread = std::jthread( OutputStats );

    read_thread.join();

    cleanup_thread.request_stop();
    cleanup_thread.join();
    parse_thread.request_stop();
    parse_thread.join();
    aggregate_thread.request_stop();
    aggregate_thread.join();
    output_thread.request_stop();
    output_thread.join();

    //OutputStats();

    return 0;
}
