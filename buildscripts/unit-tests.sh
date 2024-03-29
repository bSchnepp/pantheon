#!/bin/sh

rm -rf build
mkdir build
cd build
scan-build -plist --force-analyze-debug-code -analyze-headers -o analyzer_reports cmake .. -DONLY_TESTS=ON -DCMAKE_C_FLAGS="-fprofile-arcs -ftest-coverage -O3 -g3 -pthread -latomic" -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage -O3 -g3 -pthread -latomic" -DCMAKE_CXX_LINK_FLAGS="${CMAKE_CXX_FLAGS} -latomic -lpthread" -DCMAKE_C_LINK_FLAGS="${CMAKE_C_FLAGS} -latomic -lpthread"
scan-build -plist --force-analyze-debug-code -analyze-headers -o analyzer_reports make -j`nproc`
valgrind --xml=yes --xml-file=valgrind.xml ./pantheon --gtest_output=xml:./gtests.xml
cd ..
gcovr --exclude-directories externals/  --exclude-directories arch/ --exclude-directories board/ -e tests/struct_tests.hpp -e tests/sched_tests.hpp -e tests/common_tests.hpp -e tests/main.cpp -e tests/arch_tests.hpp -r . --xml-pretty > build/coverage.xml
sonar-scanner