#!/bin/bash

set -ex

DEP_MODULE=fido
. $HMAKE_PROJECT_DIR/deps/functions.sh

clean_copy_src
find src -name '*.cpp' -exec sed -i -r 's/std::cout/std::cerr/g' '{}' ';'

case "$ARCH" in
    armhf) export CXX=arm-linux-gnueabihf-g++ ;;
esac

make $MAKE_OPTS install DESTDIR=./_install
mkdir -p $OUT_DIR/include $OUT_DIR/lib
cp -rf _install/usr/local/include/Fido $OUT_DIR/include/
cp -f _install/usr/local/lib/fido.a $OUT_DIR/lib/libfido.a
