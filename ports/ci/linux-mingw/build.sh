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

BUILDSCRIPT=dockerbuild.sh
SOURCES_DIR=${PWD}
INSTALL_PREFIX=${SOURCES_DIR}/webcamoid-data

sudo mount --bind root.x86_64 root.x86_64
sudo mount --bind "$HOME" "root.x86_64/$HOME"
cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
cd "${SOURCES_DIR}"
echo
echo "Building x64 virtual camera driver"
echo
mkdir build-x64
x86_64-w64-mingw32-cmake \
    -S . \
    -B build-x64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"
cmake --build build-x64
cmake --build build-x64 --target install
echo
echo "Building x86 virtual camera driver"
echo
mkdir build-x86
cd build-x86
i686-w64-mingw32-cmake \
    -S . \
    -B build-x86 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"
cmake --build build-x86
cmake --build build-x86 --target install
EOF
chmod +x ${BUILDSCRIPT}
sudo cp -vf ${BUILDSCRIPT} "root.x86_64/$HOME/"

EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
${EXEC} bash "$HOME/${BUILDSCRIPT}"

sudo umount "root.x86_64/$HOME"
sudo umount root.x86_64
