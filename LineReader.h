#pragma once

#include "common.h"
#include "utils.h"

//
// Implements reading of lines from STDIN into line buckets keeping immediate processing to a minimum.
// The only processing done is to guarantee that lines of an event are not split to different buckets.
//
class CLineReader {

    protected:

        bool m_bPrevEmptyLine = false;
        PLineBucket m_current_line_bucket;
        PContext m_context;

        void FlushLineBuckets() const;
        void ReadLine();

        explicit CLineReader( PContext context );

    public:

        static void Run( const std::stop_token & stoken, const PContext & context );

};
