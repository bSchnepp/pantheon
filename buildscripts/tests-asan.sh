#!/bin/sh

rm -rf build
mkdir build
cd build
cmake .. -DONLY_TESTS=ON -DCMAKE_C_FLAGS="-O3 -g3 -pthread -latomic -fsanitize=address,undefined" -DCMAKE_CXX_FLAGS="-O3 -g3 -pthread -latomic -fsanitize=address,undefined" -DCMAKE_CXX_LINK_FLAGS="${CMAKE_CXX_FLAGS} -latomic -lpthread" -DCMAKE_C_LINK_FLAGS="${CMAKE_C_FLAGS} -latomic -lpthread"
make -j`nproc`
./pantheon