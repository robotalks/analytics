#!/bin/bash

set -ex

DEP_MODULE=daemon
. $HMAKE_PROJECT_DIR/deps/functions.sh

clean_copy_src
sed -i -r 's/^((CC|AR|RANLIB)\s*:=\s*)/\1$(CROSS_COMPILE)/g' Makefile

case "$ARCH" in
    armhf)
        export CROSS_COMPILE=arm-linux-gnueabihf-
        ;;
esac

export CCFLAGS=-fPIC
export DAEMON_LDFLAGS=-static
make
mkdir -p $OUT_DIR/bin
${CROSS_COMPILE}strip -g -o $OUT_DIR/bin/daemon daemon
chmod a+rx $OUT_DIR/bin/daemon
