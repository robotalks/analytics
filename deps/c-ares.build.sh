#!/bin/bash

set -ex

DEP_MODULE=c-ares
. $HMAKE_PROJECT_DIR/deps/functions.sh

clean_copy_src

CONFIG_OPTS=
case "$ARCH" in
    armhf) CONFIG_OPTS=--host=arm-linux-gnueabihf ;;
esac

./configure $CONFIG_OPTS \
    --disable-shared --disable-debug --disable-silent-rules \
    --enable-optimize --with-pic
make
make install DESTDIR=$BLD_DIR/_install
mkdir -p $OUT_DIR
cp -rf _install/usr/local/include $OUT_DIR/
cp -rf _install/usr/local/lib $OUT_DIR/
