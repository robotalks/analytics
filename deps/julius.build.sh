#!/bin/bash

set -ex

DEP_MODULE=julius
. $HMAKE_PROJECT_DIR/deps/functions.sh

clean_copy_src

HOST_OPT=
case "$ARCH" in
    armhf) HOST_OPT="--host=arm-linux-gnueabihf" ;;
esac

cd $BLD_DIR
patch < $HMAKE_PROJECT_DIR/deps/julius.patch
LDFLAGS=-L$OUT_DIR/lib \
CFLAGS=-I$OUT_DIR/include \
./configure $HOST_OPT \
    --prefix=$OUT_DIR/ \
    --enable-words-int \
    --enable-msd \
    --enable-gmm-vad \
    --enable-decoder-vad \
    --enable-power-reject \
    --enable-setup=standard \
    --enable-factor2 \
    --enable-wpair \
    --enable-wpair-nlimit \
    --enable-word-graph
make $MAKE_OPTS
make install
