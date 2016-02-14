echo "This script is not supposed to work. Please port the BASH version to Windows!"
cmake -DCMAKE_TOOLCHAIN_FILE="%KSDK_DIR%/tools/cmake_toolchain_files/armgcc.cmake" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug  .
mingw32-make -j4
cmake -DCMAKE_TOOLCHAIN_FILE="%KSDK_DIR%/tools/cmake_toolchain_files/armgcc.cmake" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release  .
mingw32-make -j4
pause
