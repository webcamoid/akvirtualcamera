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

export PATH="/c/Program Files (x86)/NSIS:/c/msys64/mingw64/bin:/mingw64/bin:${PATH}"
export INSTALL_PREFIX="${PWD}/package-data-${COMPILER}"
export PACKAGES_DIR="${PWD}/packages/windows-${COMPILER}"
export BUILD_PATH="${PWD}/build-${COMPILER}-x64"
export PYTHONPATH="${PWD}/DeployTools"

/mingw32/bin/strip "${INSTALL_PREFIX}/x86"/*
/mingw64/bin/strip "${INSTALL_PREFIX}/x64"/*
mkdir -p "${PACKAGES_DIR}"

python3 DeployTools/deploy.py \
    -d "${INSTALL_PREFIX}" \
    -c "${BUILD_PATH}/package_info.conf" \
    -o "${PACKAGES_DIR}"
