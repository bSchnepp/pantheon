#!/bin/sh

rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/qemu-aarch64.cmake -DCMAKE_CXX_FLAGS="-O2"
make -j`nproc`
qemu-system-aarch64 -machine virt,gic-version=2,secure=on -smp 1 -cpu cortex-a72 -kernel ./kernel.img -d int

