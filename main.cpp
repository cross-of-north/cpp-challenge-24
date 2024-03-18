#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <ranges>
#include <set>
#include <shared_mutex>
#include <thread>
#include <vector>
#include <fstream>

constexpr time_t SECONDS_PER_LINE_BUFFER = 1;
constexpr time_t SECONDS_PER_EVENT_BUFFER = 1;
constexpr time_t SECONDS_PER_OUTPUT = 60;
constexpr time_t REQUEST_LIFETIME_IN_SECONDS = 20;

//#define DEBUG_MEMORY_CONSUMPTION 1
bool DEBUG_OUTPUT = true;

bool DUMP_TO_STDOUT = true;
std::string filename;

auto to_stream( const time_t tp ) {
    return std::put_time( std::localtime( &tp ), "%F %T %Z" );
}

class CLineBuffer {

    private:

        typedef std::vector < std::string > t_lines;
        t_lines m_lines;
        time_t m_timestamp = 0;

    public:

        CLineBuffer() = delete;
        explicit CLineBuffer( const time_t ts ) : m_timestamp( ts ) {
#ifdef DEBUG_MEMORY_CONSUMPTION
            std::cout << m_timestamp << " CLineBuffer()" << std::endl;
#endif // DEBUG_MEMORY_CONSUMPTION
        }

        ~CLineBuffer() {
#ifdef DEBUG_MEMORY_CONSUMPTION
            std::cout << m_timestamp << " ~CLineBuffer()" << std::endl;
#endif // DEBUG_MEMORY_CONSUMPTION
        }

        void Push( const std::string & line ) {
            m_lines.push_back( line );
        }

        unsigned int GetCount() const {
            return m_lines.size();
        }

        const std::string & GetItem( const unsigned int n ) const {
            return m_lines[ n ];
        }

        time_t GetTimestamp() const {
            return m_timestamp;
        }
};
typedef std::shared_ptr < CLineBuffer > PLineBuffer;

class CEventBuffer {

    private:

        typedef std::tuple < time_t, std::string > t_event;
        typedef std::map < std::string, t_event > t_event_map;

        t_event_map m_events;
        mutable std::shared_mutex m_mutex;

    public:

        CEventBuffer() {
#ifdef DEBUG_MEMORY_CONSUMPTION
            std::cout << "CEventBuffer()" << std::endl;
#endif // DEBUG_MEMORY_CONSUMPTION
        }

        ~CEventBuffer() {
#ifdef DEBUG_MEMORY_CONSUMPTION
            std::cout << "~CEventBuffer()" << std::endl;
#endif // DEBUG_MEMORY_CONSUMPTION
        }

        void Push( const time_t ts, const std::string & id, const std::string & s ) {
            std::unique_lock < std::shared_mutex > lock( m_mutex );
            m_events.emplace( std::make_pair( id, std::make_tuple( ts, s ) ) );
        }

        bool GetByID( const std::string & id, std::string & value, time_t & ts ) const {
            bool bResult = false;
            value.clear();
            ts = 0;
            std::shared_lock < std::shared_mutex > lock( m_mutex );
            const auto & it = m_events.find( id );
            if ( it != m_events.end() ) {
                ts = std::get< 0 >( it->second );
                value = std::get< 1 >( it->second );
                bResult = true;
            }
            return bResult;
        }

        bool Pop( std::string & id, std::string & value, time_t & ts ) {
            bool bResult = false;
            id.clear();
            value.clear();
            ts = 0;
            std::unique_lock < std::shared_mutex > lock( m_mutex );
            const auto & it = m_events.begin();
            if ( it != m_events.end() ) {
                id = it->first;
                ts = std::get< 0 >( it->second );
                value = std::get< 1 >( it->second );
                m_events.erase( it );
                bResult = true;
            }
            return bResult;
        }

};
typedef std::shared_ptr < CEventBuffer > PEventBuffer;

template < typename T > class CTimeKeyedCollection {

    public:

        typedef std::shared_ptr < T > PT;
        typedef PT t_item;
        typedef std::map < time_t, PT > t_container;

    protected:

        t_container m_container;
        mutable std::shared_mutex m_mutex;
        std::mutex m_new_data_mutex;
        std::condition_variable m_new_data_available;

        void DoGetItemByKey( const time_t time_key, PT & item, const bool bCanAdd ) {
            item.reset();
            if (
                auto it = m_container.find( time_key );
                it == m_container.end()
            ) {
                if ( bCanAdd ) {
                    item = std::make_shared < T >();
                    m_container.emplace( std::make_pair( time_key, item ) );
                }
            } else {
                item = it->second;
            }
        }

    public:

        void GetItemByKey( const time_t ts, PT & item ) {
            {
                bool bCanAdd = false;
                std::shared_lock < std::shared_mutex > lock( m_mutex );
                DoGetItemByKey( ts, item, bCanAdd );
            }
            if ( !item ) {
                // need to add?
                bool bCanAdd = true;
                std::unique_lock < std::shared_mutex > lock( m_mutex );
                DoGetItemByKey( ts, item, bCanAdd );
            }
        }

        bool GetOldest( PT & item, time_t & ts ) const {
            item.reset();
            ts = 0;
            std::shared_lock < std::shared_mutex > lock( m_mutex );
            auto it = m_container.begin();
            if ( it != m_container.end() ) {
                ts = it->first;
                item = it->second;
            }
            return !!item;
        }

        bool GetNewest( PT & item, time_t & ts ) const {
            item.reset();
            ts = 0;
            std::shared_lock < std::shared_mutex > lock( m_mutex );
            auto it = m_container.rbegin();
            if ( it != m_container.rend() ) {
                ts = it->first;
                item = it->second;
            }
            return !!item;
        }

        bool GetNewestItem( PT & item ) const {
            time_t ts = 0;
            return GetNewest( item, ts );
        }

        bool GetNewestTimestamp( time_t & ts ) const {
            PT item;
            return GetNewest( item, ts );
        }

        bool GetOldestItem( PT & item ) const {
            time_t ts = 0;
            return GetOldest( item, ts );
        }

        bool GetOldestTimestamp( time_t & ts ) const {
            PT item;
            return GetOldest( item, ts );
        }

        void RemoveItem( const PT & item ) {
            std::unique_lock < std::shared_mutex > lock( m_mutex );
            for ( auto it = m_container.begin(); it != m_container.end(); ) {
                if ( it->second == item ) {
                    m_container.erase( it );
                    break;
                } else {
                    ++it;
                }
            }
        }

        bool IsEmpty() const {
            std::shared_lock < std::shared_mutex > lock( m_mutex );
            return m_container.empty();
        }

        void DiscardOlderThan( const time_t min_ts ) {
            std::unique_lock < std::shared_mutex > lock( m_mutex );
            for ( auto it = m_container.begin(); it != m_container.end(); ) {
                if ( it->first < min_ts ) {
                    it = m_container.erase( it );
                } else {
                    //++it;
                    break;
                }
            }
        }

        auto WaitForData( const int timeout_ms ) {
            std::unique_lock< std::mutex > lock( m_new_data_mutex );
            return m_new_data_available.wait_for( lock, std::chrono::milliseconds( timeout_ms ) );
        }

        void NotifyDataAvailable() {
            std::lock_guard < std::mutex > lock( m_new_data_mutex );
            m_new_data_available.notify_one();
        }

        void AddItem( const time_t ts, const PT & item ) {
            std::unique_lock < std::shared_mutex > lock( m_mutex );
            m_container.emplace( std::make_pair( ts, item ) );
        }

};

class CLineBuffers : public CTimeKeyedCollection < CLineBuffer > {
};

class CEventBuffers : public CTimeKeyedCollection < CEventBuffer > {

    public:

        void Push( const time_t ts, const std::string & id, const std::string & s ) {
            time_t time_key = ts / SECONDS_PER_EVENT_BUFFER;
            PEventBuffer event_buffer;
            GetItemByKey( time_key, event_buffer );
            if ( event_buffer ) {
                event_buffer->Push( ts, id, s );
            } else {
                // assert( false );
            }
        }

        bool GetByID( const std::string & id, std::string & value, time_t & ts ) const {
            bool bResult = false;
            value.clear();
            ts = 0;
            std::shared_lock < std::shared_mutex > lock( m_mutex );
            for ( const auto & [ buffer_timestamp, buffer ] : std::ranges::reverse_view( m_container ) ) {
                if ( buffer->GetByID( id, value, ts ) ) {
                    bResult = true;
                    break;
                }
            }
            return bResult;
        }

};

class CMessageProcessor {
    protected:
#ifdef _DEBUG
        std::string m_message;
#endif // _DEBUG
        std::string m_trace_id;
        std::string m_request_path;
        std::string m_result_code;
        bool m_bIsResponse = false;
        bool m_bDone = true;

        void Reset() {
#ifdef _DEBUG
            m_message.clear();
#endif // _DEBUG
            m_trace_id.clear();
            m_request_path.clear();
            m_result_code.clear();
            m_bIsResponse = false;
            m_bDone = false;
        }

        void ProcessFirstLine( const std::string & line ) {
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

        void ProcessHeaderLine( const std::string & line ) {
            static const std::string trace_id_prefix( "X-Trace-ID: " );
            if ( line.rfind( trace_id_prefix, 0 ) == 0 ) {
                m_trace_id = line.substr( trace_id_prefix.length() );
            }
        }

    public:
        void ProcessLine( const std::string & line ) {
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

        bool IsDone() const {
            return m_bDone;
        }

        bool IsResponse() const {
            return m_bIsResponse;
        }

        const std::string & GetRequestPath() const {
            return m_request_path;
        }

        const std::string & GetTraceID() const {
            return m_trace_id;
        }

        const std::string & GetResultCode() const {
            return m_result_code;
        }
};

class CAggregateStats {
    public:
        typedef std::map < std::string, std::string > t_string_to_string_map;
        typedef std::map < std::string, long long unsigned int > t_string_to_uint64_map;
        typedef std::map < std::string, t_string_to_uint64_map > t_aggregate_stats;
    protected:
        std::mutex m_mutex;
        t_aggregate_stats m_stats;
    public:

        std::mutex & GetMutex() {
            return m_mutex;
        }

        t_aggregate_stats & GetStats() {
            return m_stats;
        }
};
typedef std::shared_ptr < CAggregateStats > PAggregateStats;

class CAggregateStatsCollection : public CTimeKeyedCollection < CAggregateStats > {
    public:
        static time_t GetQuantizedTime( const time_t ts, const time_t delta = 0 ) {
            return ( ts / SECONDS_PER_OUTPUT + delta ) * SECONDS_PER_OUTPUT;
        }
};

CMessageProcessor mp;
CEventBuffers request_map;
CEventBuffers response_map;
CLineBuffers filling_line_buffers;
CLineBuffers ready_line_buffers;
CAggregateStatsCollection stats;

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

void OutputStats( const std::stop_token & stoken ) {

    bool bHadOutput = false;

    while ( true ) {

        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        time_t current_time = CAggregateStatsCollection::GetQuantizedTime( time( nullptr ) );

        bool bShouldWait = false;

        PAggregateStats stats_item;
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

            time_t min_unprocessed_time = CAggregateStatsCollection::GetQuantizedTime( stats_ts, +1 );

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
                PAggregateStats stats_item;
                time_t stats_time = 0;

                while ( buffer->Pop( id, result_code, result_ts ) ) {
                    time_t ts = 0;
                    std::string request;
                    time_t current_stats_time = CAggregateStatsCollection::GetQuantizedTime( result_ts );
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
