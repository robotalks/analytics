#!/bin/bash
set -ex

DEP_MODULE=opencv
. $HMAKE_PROJECT_DIR/deps/functions.sh

CMAKE_OPTS=
if [ "$ARCH" == "armhf" ]; then
    CMAKE_OPTS="$CMAKE_OPTS -DENABLE_VFPV3=ON -DENABLE_NEON=ON -DWITH_TBB=ON -DBUILD_TBB=ON"
    CMAKE_OPTS="$CMAKE_OPTS -DGCC_COMPILER_VERSION=4.9 -DCMAKE_TOOLCHAIN_FILE=$SRC_DIR/platforms/linux/arm-gnueabi.toolchain.cmake"
fi

clean_copy_src

patch -p1 < $HMAKE_PROJECT_DIR/deps/opencv.patch
patch -p1 < $HMAKE_PROJECT_DIR/deps/opencv-arm-tbb.patch

export PKG_CONFIG_PATH=$OUT_DIR/lib/pkgconfig
cmake $CMAKE_OPTS \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_DOCS=OFF \
    -DBUILD_JPEG=ON \
    -DBUILD_PNG=ON \
    -DBUILD_TIFF=ON \
    -DBUILD_ZLIB=ON \
    -DBUILD_JASPER=ON \
    -DBUILD_OPENEXR=ON \
    -DFFMPEG_INCLUDE_DIRS=$OUT_DIR/include \
    -DFFMPEG_LIBRARY_DIRS=$OUT_DIR/lib \
    -DCMAKE_PREFIX_PATH=/usr/local \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DOPENCV_EXTRA_MODULES_PATH=$BLD_BASE/src/opencv_contrib/modules \
    .

make $MAKE_OPTS
make install DESTDIR=./_install

mkdir -p $OUT_DIR
cp -rf _install/usr/local/* $OUT_DIR/
