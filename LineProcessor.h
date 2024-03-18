#pragma once

#include "common.h"

#include "utils.h"

class CLineProcessor {

    protected:

        PContext m_context;

        void ParseLineBuffer( const PLineBuffer & buffer );
        void ProcessLineBuffers();

        explicit CLineProcessor( PContext context );

    public:

        static void Run( const std::stop_token & stoken, const PContext & context );

};
