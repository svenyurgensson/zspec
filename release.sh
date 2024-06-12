#!/bin/bash

set -e

make

dockcross-win-x64 bash -c 'cmake -DDCMAKE_TOOLCHAIN_FILE=/usr/src/mxe/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake -S . -B "Win32" && cmake --build Win32 --config Debug'

rm releases/*.zip

zip releases/zspec.win64.zip Win32/zspec.exe
zip releases/zspec.osx.zip zspec
zip -r releases/examples.zip examples