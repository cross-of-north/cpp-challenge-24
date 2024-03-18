#pragma once

#include "common.h"

#include "utils.h"

//
// Implements conversion of lines received to request/response events, drops processed line buckets
//
class CLineProcessor {

    protected:

        PContext m_context;

        void ParseLineBucket( const PLineBucket & bucket );
        void ProcessLineBuckets();

        explicit CLineProcessor( PContext context );

    public:

        static void Run( const std::stop_token & stoken, const PContext & context );

};
