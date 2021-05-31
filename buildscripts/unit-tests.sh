#!/bin/sh

rm -rf build
mkdir build
cd build
cmake .. -DONLY_TESTS=ON -DCMAKE_C_FLAGS="-fprofile-arcs -ftest-coverage -O2" -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage -O2"
make -j`nproc`
./pantheon --gtest_output=xml:./gtests.xml
cd ..
gcovr -r . --xml-pretty > build/coverage.xml
sonar-scanner