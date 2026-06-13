#! /usr/bin/env sh
cd $1

CC=aarch64-linux-gnu-gcc \
PKG_CONFIG_PATH="$CUSTOM_PREFIX/lib/pkgconfig" \
./configure --host=aarch64-linux-gnu --enable-static --enable-shared --prefix="$CUSTOM_PREFIX"

make -j$(nproc)
sudo make install
