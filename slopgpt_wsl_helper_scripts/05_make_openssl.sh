#! /usr/bin/env sh
cd $1

./Configure linux-aarch64 \
    --cross-compile-prefix=aarch64-linux-gnu- \
    --prefix="$CUSTOM_PREFIX" \
    zlib \
    --with-zlib-lib="$CUSTOM_PREFIX/lib" \
    --with-zlib-include="$CUSTOM_PREFIX/include"

make -j$(nproc)
sudo make install_sw
