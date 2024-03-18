cmake_minimum_required(VERSION 3.28)
project(untitled)

set(CMAKE_CXX_STANDARD 20)

add_executable(processor
        main.cpp
        LineBuffer.cpp
        LineBuffer.h
        TimeKeyedCollection.h
        common.h
        utils.cpp
        utils.h
        EventBuffer.h
        EventBuffer.cpp
        AggregatedStats.cpp
        AggregatedStats.h
        MessageParser.cpp
        MessageParser.h
        OutputProcessor.cpp
        OutputProcessor.h
        Aggregator.cpp
        Aggregator.h
        LineProcessor.cpp
        LineProcessor.h
        LineReader.cpp
        LineReader.h
)
