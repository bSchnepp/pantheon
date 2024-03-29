#!/bin/sh

rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/qemu-aarch64.cmake -DCMAKE_C_FLAGS="-g3 -Ofast" -DCMAKE_CXX_FLAGS="-g3 -Ofast"
make -j`nproc`
qemu-img create -fqcow2 emmc.img 4G
qemu-system-aarch64 -machine virt,gic-version=2,secure=on -smp 8 -cpu cortex-a72 -m 8G -kernel ./kernel.img  -device sdhci-pci -device sd-card,drive=flash -drive id=flash,if=none,format=qcow2,file=emmc.img -s -S -d int

