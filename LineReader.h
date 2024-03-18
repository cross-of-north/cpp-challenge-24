#pragma once

#include "common.h"
#include "utils.h"

class CLineReader {

    protected:

        bool m_bPrevEmptyLine = false;
        PLineBuffer m_current_line_buffer;
        PContext m_context;

        void FlushLineBuffers() const;
        void ReadLine();

        explicit CLineReader( PContext context );

    public:

        static void Run( const std::stop_token & stoken, const PContext & context );

};
