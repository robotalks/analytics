#!/bin/bash

set -ex

DEP_MODULE=minit
. $HMAKE_PROJECT_DIR/deps/functions.sh

TC_PREFIX=
case "$ARCH" in
    armhf) TC_PREFIX=arm-linux-gnueabihf- ;;
esac

mkdir -p $OUT_DIR/bin
${TC_PREFIX}gcc -Os -Wall -pedantic -std=gnu99 -s -static -o $OUT_DIR/bin/minit $SRC_DIR/minit.c
