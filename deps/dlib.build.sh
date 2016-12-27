#!/bin/bash
set -ex

DEP_MODULE=dlib
. $HMAKE_PROJECT_DIR/deps/functions.sh

CMAKE_OPTS=
case "$ARCH" in
    amd64)
        CMAKE_OPTS="$CMAKE_OPTS -DUSE_SSE4_INSTRUCTIONS=ON -DUSE_SSE2_INSTRUCTIONS=ON -DUSE_AVX_INSTRUCTIONS=ON"
        ;;
    armhf)
        CMAKE_OPTS="$CMAKE_OPTS -DDLIB_NO_GUI_SUPPORT=ON"
        CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_TOOLCHAIN_FILE=/opt/kit/cmake/armhf.toolchain.cmake"
        ;;
esac

clean_mkdir

export PKG_CONFIG_PATH=$OUT_DIR/lib/pkgconfig
export OPENBLAS_HOME=$OUT_DIR
cmake $CMAKE_OPTS \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_PREFIX_PATH=/usr/local \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DDLIB_USE_BLAS=ON \
    -DDLIB_USE_LAPACK=ON \
    $SRC_DIR

make $MAKE_OPTS
make install DESTDIR=./_install

mkdir -p $OUT_DIR
cp -rf _install/usr/local/* $OUT_DIR/
