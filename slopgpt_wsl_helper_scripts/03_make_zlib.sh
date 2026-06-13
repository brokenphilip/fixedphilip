#! /usr/bin/env sh
cd $1

CC=aarch64-linux-gnu-gcc \
CHOST=aarch64-linux-gnu \
AR=aarch64-linux-gnu-ar \
RANLIB=aarch64-linux-gnu-ranlib \
./configure --prefix="$CUSTOM_PREFIX" --shared

make -j$(nproc)
sudo make install

echo zlib hopefully installed
