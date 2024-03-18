FROM alpine:3.19 AS build
RUN apk update && \
    apk add --no-cache \
        gcompat=1.1.0-r4 \
        build-base=0.5-r3 \
        cmake=3.27.8-r0 \
        mc \
        git
WORKDIR /source
RUN git clone  https://github.com/cross-of-north/cpp-challenge-24
WORKDIR /source/cpp-challenge-24/build
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --parallel 8
COPY ../generator .
CMD cd /source/cpp-challenge-24 && \
		git pull && \
		cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --parallel 8 && \
    cp ../generator . && \
		( ./generator|./processor ) && \
		/bin/sh
