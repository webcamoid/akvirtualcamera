#!/bin/bash

# akvirtualcamera, virtual camera for Mac and Windows.
# Copyright (C) 2021  Gonzalo Exequiel Pedone
#
# akvirtualcamera is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# akvirtualcamera is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with akvirtualcamera. If not, see <http://www.gnu.org/licenses/>.
#
# Web-Site: http://webcamoid.github.io/

SOURCES_DIR=${PWD}
INSTALL_PREFIX=${SOURCES_DIR}/webcamoid-data
ORIG_PATH=${PATH}

echo
echo Building x64 virtual camera driver
echo

mkdir build-x64
cd build-x64

export PATH=/c/msys64/mingw64/bin:/c/msys64/usr/bin:${ORIG_PATH}
cmake \
    -G "MSYS Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    ..
cmake --build .
cmake --build . --target install

cd ..

echo
echo Building x86 virtual camera driver
echo

mkdir build-x86
ls -l
cd build-x86

export PATH=/c/msys64/mingw32/bin:/c/msys64/usr/bin:${ORIG_PATH}
cmake \
    -G "MSYS Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    ..
cmake --build .
cmake --build . --target install
