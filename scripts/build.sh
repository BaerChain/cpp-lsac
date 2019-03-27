#!/usr/bin/env bash

set -e -x

# There is an implicit assumption here that we HAVE to run from repo root

mkdir -p build
cd build
if [ $(uname -s) == "Linux" ]; then
#	Disabling while llvm-3.9 package is broken
#    cmake .. -DCMAKE_BUILD_TYPE=$1 -DCOVERAGE=ON -DEVMJIT=ON -DLLVM_DIR=/usr/lib/llvm-3.9/lib/cmake/llvm
    cmake .. -DCMAKE_BUILD_TYPE=$1 -DCOVERAGE=ON
else
    cmake .. -DCMAKE_BUILD_TYPE=$1 -DCOVERAGE=ON
fi

make -j2
