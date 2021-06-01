#!/bin/sh

rm -rf build
mkdir build
cd build
scan-build -plist --force-analyze-debug-code -analyze-headers -o analyzer_reports cmake .. -DONLY_TESTS=ON -DCMAKE_C_FLAGS="-fprofile-arcs -ftest-coverage -O2" -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage -O2"
scan-build -plist --force-analyze-debug-code -analyze-headers -o analyzer_reports make -j`nproc`
./pantheon --gtest_output=xml:./gtests.xml
cd ..
gcovr --exclude-directories tests/ --exclude-directories externals/ -r . --xml-pretty > build/coverage.xml
sonar-scanner