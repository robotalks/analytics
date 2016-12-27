#!/bin/bash

set -ex

DEP_MODULE=bzip2
. $HMAKE_PROJECT_DIR/deps/functions.sh

TC_PREFIX=
case "$ARCH" in
    armhf) TC_PREFIX=arm-linux-gnueabihf- ;;
esac

clean_copy_src

if [ -n "$TC_PREFIX" ]; then
    sed -i -r "s/^CC=.+$/CC=${TC_PREFIX}gcc/g" Makefile
    sed -i -r "s/^AR=.+$/AR=${TC_PREFIX}ar/g" Makefile
    sed -i -r "s/^RANLIB=.+$/RANLIB=${TC_PREFIX}ranlib/g" Makefile
fi

make $MAKE_OPTS libbz2.a
mkdir -p $OUT_DIR/include $OUT_DIR/lib
cp -f bzlib.h $OUT_DIR/include/
cp -f libbz2.a $OUT_DIR/lib/
