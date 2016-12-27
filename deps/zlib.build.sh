#!/bin/bash

set -ex

DEP_MODULE=zlib
. $HMAKE_PROJECT_DIR/deps/functions.sh

case "$ARCH" in
    armhf) export CROSS_PREFIX=arm-linux-gnueabihf- ;;
esac

clean_copy_src

./configure --static
make $MAKE_OPTS
make install DESTDIR=./_install
mkdir -p $OUT_DIR
cp -rf _install/usr/local/* $OUT_DIR/
