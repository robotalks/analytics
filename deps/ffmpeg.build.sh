#!/bin/bash
set -ex

DEP_MODULE=ffmpeg
. $HMAKE_PROJECT_DIR/deps/functions.sh

CONFIG_OPTS=
case "$ARCH" in
    armhf)
        CONFIG_OPTS="$CONFIG_OPTS --enable-cross-compile --cross-prefix=arm-linux-gnueabihf- --arch=armhf --target-os=linux"
    ;;
esac

clean_mkdir

$SRC_DIR/configure $CONFIG_OPTS --disable-doc --disable-shared
make $MAKE_OPTS
make install DESTDIR=./_install
mkdir -p $OUT_DIR
cp -rf _install/usr/local/* $OUT_DIR/
rm -f $OUT_DIR/lib/libavresample.a
ln -s libswresample.a $OUT_DIR/lib/libavresample.a
