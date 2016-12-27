#!/bin/bash

set -ex

DEP_MODULE=libwebsockets
. $HMAKE_PROJECT_DIR/deps/functions.sh

clean_copy_src

CMAKE_OPTS=
if [ "$ARCH" == "armhf" ]; then
    CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_TOOLCHAIN_FILE=/opt/kit/cmake/armhf.toolchain.cmake"
fi

export PKG_CONFIG_PATH=$OUT_DIR/lib/pkgconfig
cmake $CMAKE_OPTS \
    -DLWS_WITH_HTTP2=1 \
    -DLWS_WITH_SHARED=OFF \
    -DLWS_WITHOUT_TESTAPPS=ON \
    -DLWS_WITHOUT_TEST_SERVER=ON \
    -DLWS_WITHOUT_TEST_PING=ON \
    -DLWS_WITHOUT_TEST_ECHO=ON \
    -DLWS_WITHOUT_TEST_CLIENT=ON \
    -DLWS_WITHOUT_TEST_FRAGGLE=ON \
    -DLWS_UNIX_SOCK=ON \
    -DLWS_STATIC_PIC=ON \
    -DLWS_OPENSSL_INCLUDE_DIRS=$OUT_DIR/include/openssl \
    -DLWS_OPENSSL_LIBRARIES="$OUT_DIR/lib/libssl.a;$OUT_DIR/lib/libcrypto.a;$OUT_DIR/lib/libz.a" \
    -DLWS_ZLIB_LIBRARIES="$OUT_DIR/lib/libz.a" \
    -DLWS_ZLIB_INCLUDE_DIRS="$OUT_DIR/include" \
    -DCMAKE_FIND_ROOT_PATH="$OUT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    .

make $MAKE_OPTS
make install DESTDIR=./_install
mkdir -p $OUT_DIR
cp -rf _install/usr/local/include $OUT_DIR/
cp -rf _install/usr/local/lib $OUT_DIR/
