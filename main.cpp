#include <iostream>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <vector>

constexpr time_t SECONDS_PER_LINE_BUFFER = 1;
constexpr time_t SECONDS_PER_EVENT_BUFFER = 1;

class CLineBuffer {

    private:

        typedef std::vector < std::string > t_lines;
        t_lines m_lines;

    public:

        void Push( const std::string & line ) {
            m_lines.push_back( line );
        }

        unsigned int GetCount() const {
            return m_lines.size();
        }

        const std::string & GetItem( const unsigned int n ) const {
            return m_lines[ n ];
        }
};
typedef std::shared_ptr < CLineBuffer > PLineBuffer;

typedef std::map < time_t, PLineBuffer > t_line_buffers;

class CEventBuffer {

    private:

        typedef std::tuple < time_t, std::string > t_event;
        typedef std::map < std::string, t_event > t_event_map;

        t_event_map m_events;

    public:

        void Push( const time_t ts, const std::string & id, const std::string & s ) {
            m_events.emplace( std::make_pair( id, std::make_tuple( ts, s ) ) );
        }

        bool GetByID( const std::string & id, std::string & value, time_t & ts ) const {
            bool bResult = false;
            value.clear();
            ts = 0;
            const auto & it = m_events.find( id );
            if ( it != m_events.end() ) {
                ts = std::get< 0 >( it->second );
                value = std::get< 1 >( it->second );
                bResult = true;
            }
            return bResult;
        }

};
typedef std::shared_ptr < CEventBuffer > PEventBuffer;
typedef std::map < time_t, PEventBuffer > t_event_buffers;

class CEventBuffers {
    private:
        t_event_buffers m_event_buffers;
    public:
        void Push( const time_t ts, const std::string & id, const std::string & s ) {
            time_t time_key = ts / SECONDS_PER_EVENT_BUFFER;
            PEventBuffer event_buffer;
            if (
                auto it = m_event_buffers.find( time_key );
                it == m_event_buffers.end()
            ) {
                event_buffer = std::make_shared < CEventBuffer >();
                m_event_buffers.emplace( std::make_pair( time_key, event_buffer ) );
            } else {
                event_buffer = it->second;
            }
            event_buffer->Push( ts, id, s );
        }

        bool GetByID( const std::string & id, std::string & value, time_t & ts ) const {
            bool bResult = false;
            value.clear();
            ts = 0;
            for ( const auto & [ buffer_timestamp, buffer ] : std::ranges::reverse_view( m_event_buffers ) ) {
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
        time_t m_timestamp = time( nullptr );
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

typedef std::map < std::string, std::string > t_string_to_string_map;
typedef std::map < std::string, long long unsigned int > t_string_to_uint64_map;
typedef std::map < std::string, t_string_to_uint64_map > t_string_to_string_to_uint64_map;

int main() {

    CMessageProcessor mp;
    CEventBuffers request_map;
    t_string_to_string_to_uint64_map result_map;
    t_line_buffers line_buffers;

    PLineBuffer current_line_buffer;
    time_t current_line_buffer_timestamp = 0;
    while ( std::cin.good() ) {
        if (
            time_t ts = time( nullptr ) / SECONDS_PER_LINE_BUFFER;
            ts != current_line_buffer_timestamp
        ) {
            current_line_buffer = std::make_shared < CLineBuffer >();
            line_buffers[ ts ] = current_line_buffer;
            current_line_buffer_timestamp = ts;
        }
        std::string input;
        std::getline( std::cin, input );
        current_line_buffer->Push( input );
        //std::cout << input << std::endl;
    }

    for ( const auto & [ buffer_timestamp, buffer ] : line_buffers ) {
        unsigned int line_count = buffer->GetCount();
        for ( unsigned int i = 0; i < line_count; i++ ) {
            mp.ProcessLine( buffer->GetItem( i ) );
            if ( mp.IsDone() ) {
                if ( mp.IsResponse() ) {
                    time_t ts = 0;
                    std::string request;
                    if ( request_map.GetByID( mp.GetTraceID(), request, ts ) ) {
                        //std::cout << "Response: " << mp.GetTraceID() << " " << mp.GetResultCode() << " " << request << std::endl;
                        result_map[ request ][ mp.GetResultCode() ]++;
                    } else {
                        result_map[ "undefined" ][ mp.GetResultCode() ]++;
                    }
                } else {
                    request_map.Push( buffer_timestamp, mp.GetTraceID(), mp.GetRequestPath() );
                    //std::cout << "Request: " << mp.GetTraceID() << " " << mp.GetRequestPath() << std::endl;
                }
            }
        }
    }

    std::set < std::string > result_codes;
    for ( const auto & stats : std::views::values( result_map ) ) {
        const auto & request_result_codes = std::views::keys( stats );
        result_codes.insert( request_result_codes.begin(), request_result_codes.end() );
    }

    const char * csv_separator = ";";//"\t";

    std::cout << "request";
    for ( const auto & result_code : result_codes ) {
        std::cout << csv_separator << result_code;
    }
    for ( auto & [ request, stats ] : result_map ) {
        std::cout << std::endl;
        std::cout << request;
        for ( const auto & result_code : result_codes ) {
            std::cout << csv_separator << stats[ result_code ];
        }
    }

    return 0;
}
