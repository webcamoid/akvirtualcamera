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

git clone https://github.com/webcamoid/DeployTools.git

cat << EOF > package_info_strip.conf
[System]
stripCmd = x86_64-w64-mingw32-strip
EOF

export WINEPREFIX=/opt/.wine
export PATH="${PWD}/.local/bin:${PATH}"
export INSTALL_PREFIX="${PWD}/package-data-${COMPILER}"
export PACKAGES_DIR="${PWD}/packages/windows-${COMPILER}"
export PYTHONPATH="${PWD}/DeployTools"
export BUILD_PATH="${PWD}/build-${COMPILER}-x64"

i686-w64-mingw32-strip ${INSTALL_PREFIX}/x86/*
x86_64-w64-mingw32-strip ${INSTALL_PREFIX}/x64/*

python ./DeployTools/deploy.py \
    -d "${INSTALL_PREFIX}" \
    -c "${BUILD_PATH}/package_info.conf" \
    -c "${PWD}/package_info_strip.conf" \
    -o "${PACKAGES_DIR}"
