#!/bin/sh

rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/qemu-aarch64.cmake -DCMAKE_C_FLAGS="-g3 -Og" -DCMAKE_CXX_FLAGS="-g3 -Og"
make -j`nproc`
qemu-system-aarch64 -machine virt,gic-version=2 -cpu cortex-a72 -kernel ./kernel.img -s -S

