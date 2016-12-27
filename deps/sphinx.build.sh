#!/bin/bash

set -ex

DEP_MODULE=sphinx
. $HMAKE_PROJECT_DIR/deps/functions.sh

clean_copy_src

HOST_OPT=
case "$ARCH" in
    armhf) HOST_OPT="--host=arm-linux-gnueabihf" ;;
esac

cd $BLD_DIR/sphinxbase
./configure $HOST_OPT --disable-shared --enable-static --without-python
make $MAKE_OPTS
make install DESTDIR=`pwd`/_install

cd $BLD_DIR/pocketsphinx
./configure $HOST_OPT --disable-shared --enable-static --without-python
make $MAKE_OPTS
make install DESTDIR=`pwd`/_install

mkdir -p $OUT_DIR
cp -rf $BLD_DIR/sphinxbase/_install/usr/local/* $OUT_DIR/
cp -rf $BLD_DIR/pocketsphinx/_install/usr/local/* $OUT_DIR/
