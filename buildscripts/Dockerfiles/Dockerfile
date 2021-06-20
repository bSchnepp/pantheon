FROM debian:buster-slim

# The host side essentials we really need
RUN apt-get update && apt-get upgrade -y && apt-get install -y make wget clang lld git clang-tidy valgrind clang-tools libssl-dev

# CMake needs to be build from source (for gtest)
RUN mkdir -p /src
RUN cd /src && wget "https://github.com/Kitware/CMake/releases/download/v3.20.4/cmake-3.20.4.tar.gz" && tar xf "cmake-3.20.4.tar.gz"
RUN cd /src/cmake-3.20.4 && ./bootstrap && make -j`nproc` && make install

# GTest needs to be brought in manually, for consistency.
RUN cd /src && wget "https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz" && tar xf "release-1.11.0.tar.gz"
RUN cd /src/googletest-release-1.11.0 && cmake . && make -j`nproc` && make install

# We also need to build gcc ourselves.
RUN apt-get install -y build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev

ARG PREFIX="/usr"
ARG TARGET="aarch64-none-elf"

# Fetch the dependencies we need
RUN cd /src && wget "https://ftp.gnu.org/gnu/binutils/binutils-2.36.1.tar.xz" && tar xf "binutils-2.36.1.tar.xz"
RUN cd /src && wget "https://ftp.gnu.org/gnu/gcc/gcc-11.1.0/gcc-11.1.0.tar.xz" && tar xf "gcc-11.1.0.tar.xz"

# Build binutils
RUN mkdir /src/binutils-build
RUN cd /src/binutils-build && /src/binutils-2.36.1/configure --target="$TARGET" --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
RUN cd /src/binutils-build && make -j`nproc`
RUN cd /src/binutils-build && make install

# Build gcc
RUN mkdir /src/gcc-build
RUN cd /src/gcc-build && /src/gcc-11.1.0/configure --target="$TARGET" --prefix="$PREFIX" --without-headers --enable-languages=c,c++ --disable-nls 
RUN cd /src/gcc-build && make -j`nproc` all-gcc
RUN cd /src/gcc-build && make -j`nproc` install-gcc

RUN cd /src/gcc-build && make -j`nproc` all-target-libgcc
RUN cd /src/gcc-build && make -j`nproc` install-target-libgcc

# We're done.
