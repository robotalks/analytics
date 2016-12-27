#!/bin/bash
set -ex

DEP_MODULE=openblas
. $HMAKE_PROJECT_DIR/deps/functions.sh

OPTS="NO_SHARED=1"
case "$ARCH" in
    amd64)
        OPTS="$OPTS TARGET=SANDYBRIDGE BINARY=64"
        ;;
    armhf)
        OPTS="$OPTS TARGET=ARMV7 BINARY=32 HOSTCC=gcc CC=arm-linux-gnueabihf-gcc FC=arm-linux-gnueabihf-gfortran"
        ;;
esac

clean_copy_src

make $MAKE_OPTS $OPTS
make install $MAKE_OPTS $OPTS PREFIX=$OUT_DIR/
