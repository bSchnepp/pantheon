#!/bin/sh

rm -rf build
mkdir build
cd build
cmake .. -DONLY_TESTS=ON -DCMAKE_C_FLAGS="-fprofile-arcs -ftest-coverage -O2 -g3" -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage -O2 -g3"
make -j`nproc`
./pantheon