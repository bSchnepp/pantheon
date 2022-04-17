#!/bin/sh

rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/raspi4.cmake -DCMAKE_CXX_FLAGS="-O2"
make -j`nproc`

