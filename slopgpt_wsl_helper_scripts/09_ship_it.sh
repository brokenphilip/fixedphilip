#! /usr/bin/env sh
echo patching
patchelf /home/filip/.vs/fixedphilip/out/build/aarch64-debug-linux/fixedphilip --set-rpath '$ORIGIN/libs'
readelf -d /home/filip/.vs/fixedphilip/out/build/aarch64-debug-linux/fixedphilip | grep -E "RPATH|RUNPATH"
echo sending libs
rsync /home/filip/.vs/fixedphilip/out/build/aarch64-debug-linux/DPP/library/lib* filip@armbian:/home/filip/fixedphilip/libs -r -v
rsync /home/filip/fixedphilip/libs/lib/lib* filip@armbian:/home/filip/fixedphilip/libs -r -v
echo sending fixedphilip
rsync /home/filip/.vs/fixedphilip/out/build/aarch64-debug-linux/fixedphilip filip@armbian:/home/filip/fixedphilip/fixedphilip
echo glhf
