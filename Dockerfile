# How to run:
# 1. Download the Dockerfile
# $ wget https://raw.githubusercontent.com/banach-space/llvm-tutor/master/Dockerfile
# 2. Build the Docker image
# $ docker build -t=llvm-tutor:llvm-12 .
# 3. Run the Docker container
# $ docker run --rm -it --hostname=llvm-tutor llvm-tutor:llvm-12 /bin/bash

FROM ubuntu:bionic

RUN apt-get update -y
RUN apt-get install --reinstall -y locales
# uncomment chosen locale to enable it's generation
RUN sed -i 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen
# generate chosen locale
RUN locale-gen en_US.UTF-8
# set system-wide locale settings 
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US
ENV LC_ALL en_US.UTF-8

RUN apt-get update && apt-get install -y software-properties-common gnupg wget
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
RUN apt-add-repository "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-12 main"
RUN apt-get update
RUN apt-get install -y llvm-12 llvm-12-dev clang-12 llvm-12-tools python3-setuptools

# Get latest cmake
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
RUN echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null

# Installing dependencies
RUN apt-get update && apt-get install -y \
    git \
    cmake \
    ninja-build \
    build-essential \
    python3-minimal python3-pip\
    && rm -rf /var/lib/apt/lists/*

# Installing lit
# Note that lit's tests depend on 'not' and 'FileCheck', LLVM utilities.
# https://github.com/llvm/llvm-project/tree/master/llvm/utils/lit
# So, we need to add -DLLVM_INSTALL_UTILS=ON cmake flag when trying to build LLVM.
# https://llvm.org/docs/CMake.html
RUN pip3 install lit
RUN pip3 install py-gnuplot

ENV LLVM_DIR /usr/lib/llvm-12

# Building LLVM+Clang (release/12.x) from source
#ENV LLVM_DIR /opt/llvm
#RUN git clone --branch release/12.x --depth 1 https://github.com/llvm/llvm-project \
#    && mkdir -p $LLVM_DIR \
#    && mkdir -p llvm-project/build \
#    && cd llvm-project/build \
#    && cmake -G Ninja \
#        -DLLVM_ENABLE_PROJECTS=clang \
#        -DLLVM_TARGETS_TO_BUILD=X86 \
#        -DCMAKE_BUILD_TYPE=Release \
#        -DCMAKE_INSTALL_PREFIX=$LLVM_DIR \
#        -DLLVM_INSTALL_UTILS=ON \
#        ../llvm \
#    && cmake --build . --target install \
#    && rm -r /llvm-project

# Building llvm-tutor
#RUN git clone https://github.com/m7nu3l/llvm-tutor/ $TUTOR_DIR \
#    && mkdir -p $TUTOR_DIR/build \
#    && cd $TUTOR_DIR/build \
#    && cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ../ \
#    && make -j $(nproc --all) \
#    && lit test/

RUN apt-get update && apt-get install -y gnuplot

# Build our sources
COPY . /llvm-tutor
ENV TUTOR_DIR /llvm-tutor
RUN rm -r -f $TUTOR_DIR/build \
    && mkdir -p $TUTOR_DIR/build \
    && cd $TUTOR_DIR/build \
    && cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ../ \
    && make -j $(nproc --all) \
    && lit -vv test/

# Profile 'zip' open-source project 
RUN git clone https://github.com/kuba--/zip zip-profiled \
    && cd zip-profiled \
    && mkdir build \
    && cd build \
    && CC="clang-12 -O2 -fexperimental-new-pass-manager -fpass-plugin=/llvm-tutor/build/lib/libProfiler.so" CXX="clang++-12 -O2 -fexperimental-new-pass-manager -fpass-plugin=/llvm-tutor/build/lib/libProfiler.so" cmake /zip-profiled \
    && make -j4 \
    && LD_PRELOAD=/llvm-tutor/build/lib/libProfilerRuntime.so ctest -VV > test.profiled.log \
    && tail -n 50 test.profiled.log

# Histogram 'zip' open-source project 
RUN git clone https://github.com/kuba--/zip zip-histogramed \
    && cd zip-histogramed \
    && mkdir build \
    && cd build \
    && CC="clang-12 -O2 -fexperimental-new-pass-manager -fpass-plugin=/llvm-tutor/build/lib/libHistogram.so" CXX="clang++-12 -O2 -fexperimental-new-pass-manager -fpass-plugin=/llvm-tutor/build/lib/libHistogram.so" cmake /zip-histogramed \
    && make -j4 \
    && tail -n 25 /zip-histogramed/build/histogram.data \
    && python3 /llvm-tutor/scripts/draw-histogram.py /zip-histogramed/build/histogram.data /zip-histogramed/build/histogram.png