#include <iostream>

class CMessageProcessor {
    protected:
#ifdef _DEBUG
        std::string m_message;
#endif // _DEBUG
        std::string m_trace_id;
        std::string m_request_path;
        std::string m_result_code;
        bool m_bIsResponse = false;
        time_t m_timestamp = 0;
        bool m_bDone = true;

        void Reset() {
#ifdef _DEBUG
            m_message.clear();
#endif // _DEBUG
            m_trace_id.clear();
            m_request_path.clear();
            m_result_code.clear();
            m_bIsResponse = false;
            m_timestamp = time( nullptr );
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
};

int main() {
    while ( std::cin.good() ) {
        std::string input;
        std::getline( std::cin, input );
        std::cout << input << std::endl;
    }
    return 0;
}
