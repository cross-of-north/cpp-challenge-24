#pragma once

#include "common.h"

class CLineReader {

    protected:

        static void FlushLineBuffers();

    public:

        static void Run( const std::stop_token & stoken );

};
