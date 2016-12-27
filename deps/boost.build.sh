#!/bin/bash

set -ex

DEP_MODULE=boost
. $HMAKE_PROJECT_DIR/deps/functions.sh

clean_copy_src

B2_VARS=--debug-configuration
case "$ARCH" in
    armhf)
        echo 'using gcc : arm : arm-linux-gnueabihf-g++ ;' >user-config.jam
        B2_VARS="$B2_VARS --user-config=user-config.jam toolset=gcc-arm"
    ;;
esac

./bootstrap.sh --prefix=$OUT_DIR
./b2 -a -q -d+2 $B2_OPTS \
    --without-python \
    --without-context \
    --without-coroutine \
    --without-coroutine2 \
    --without-wave \
    --without-mpi \
    --without-graph \
    --without-graph_parallel \
    --without-test \
    target-os=linux threading=multi threadapi=pthread \
    variant=release link=static $B2_VARS \
    install \
    -s ZLIB_BINARY=z \
    -s ZLIB_INCLUDE=$OUT_DIR/include \
    -s ZLIB_LIBPATH=$OUT_DIR/lib \
    -s BZIP2_BINARY=bz2 \
    -s BZIP2_INCLUDE=$OUT_DIR/include \
    -s BZIP2_LIBPATH=$OUT_DIR/lib
