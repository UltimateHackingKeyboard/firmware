#!/bin/sh
. ./ksdk_dir.sh
cmake -DCMAKE_TOOLCHAIN_FILE="$KSDK_DIR/tools/cmake_toolchain_files/armgcc.cmake" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release  .
make -j4
