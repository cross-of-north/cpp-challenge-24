#pragma once

#include "common.h"

#include "utils.h"

class CLineProcessor {

    protected:

        static void ParseLineBuffer( const PLineBuffer & buffer );
        static void ProcessLineBuffers();

    public:

        static void Run( const std::stop_token & stoken );

};
