FROM ubuntu:focal

# The host side essentials we really need
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get upgrade -y && DEBIAN_FRONTEND=noninteractive apt-get install -y make wget clang git valgrind libssl-dev gcovr unzip libz-dev

# Also get the gcc dependencies
RUN apt-get install -y build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev

RUN mkdir -p /src

# Obtain all the dependencies we need from source
RUN cd /src && wget "https://github.com/Kitware/CMake/releases/download/v3.20.4/cmake-3.20.4.tar.gz" && tar xf "cmake-3.20.4.tar.gz"
RUN cd /src && wget "https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz" && tar xf "release-1.11.0.tar.gz"
RUN cd /src && wget "https://ftp.gnu.org/gnu/binutils/binutils-2.36.1.tar.xz" && tar xf "binutils-2.36.1.tar.xz"
RUN cd /src && wget "https://ftp.gnu.org/gnu/gcc/gcc-11.1.0/gcc-11.1.0.tar.xz" && tar xf "gcc-11.1.0.tar.xz"

RUN cd /src && git clone https://github.com/llvm/llvm-project.git
RUN cd /src/llvm-project && git checkout d28af7c654d8db0b68c175db5ce212d74fb5e9bc

# For CI, sonar-scanner is needed too.
RUN cd /src && wget "https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-4.6.2.2472-linux.zip" && unzip "sonar-scanner-cli-4.6.2.2472-linux.zip" && cp -r sonar-scanner-4.6.2.2472-linux/* /usr

ARG PREFIX="/usr"
ARG TARGET="aarch64-none-elf"

# CMake needs to be built from source (for gtest)
RUN cd /src/cmake-3.20.4 && ./bootstrap && make -j`nproc` && make install

# GTest needs to be brought in manually, for consistency.
RUN cd /src/googletest-release-1.11.0 && cmake . -DCMAKE_CXX_FLAGS="-fPIE -g3 -pthread" && make -j`nproc` && make install

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

# Get a much newer version of clang/llvm
RUN cd /src/llvm-project && mkdir build && cd build && cmake -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;libcxx;libcxxabi;lld" -DCMAKE_BUILD_TYPE=Release ../llvm
RUN cd /src/llvm-project/build && make -j`nproc` && make install

# We're done.
