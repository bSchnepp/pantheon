#!/bin/sh

rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/qemu-aarch64.cmake
make -j`nproc`
qemu-system-aarch64 -machine virt,gic-version=2 -smp 8 -cpu cortex-a72 -kernel ./kernel.img -d int

