@echo off
setlocal
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Release

endlocal