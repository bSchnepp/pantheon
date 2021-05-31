#!/bin/sh

rm -rf build
mkdir build
cd build
cmake .. -DONLY_TESTS=ON
make -j`nproc`
./pantheon