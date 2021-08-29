#!/bin/sh

cd build
qemu-system-aarch64 -machine virt,gic-version=2,secure=on -smp 8 -m 8G -cpu cortex-a72 -kernel ./kernel.img -d int