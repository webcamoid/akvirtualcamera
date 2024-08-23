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

if [ ! -z "${GITHUB_SHA}" ]; then
    export GIT_COMMIT_HASH="${GITHUB_SHA}"
elif [ ! -z "${CIRRUS_CHANGE_IN_REPO}" ]; then
    export GIT_COMMIT_HASH="${CIRRUS_CHANGE_IN_REPO}"
fi

export GIT_BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)

if [ -z "${GIT_BRANCH_NAME}" ]; then
    if [ ! -z "${GITHUB_REF_NAME}" ]; then
        export GIT_BRANCH_NAME="${GITHUB_REF_NAME}"
    elif [ ! -z "${CIRRUS_BRANCH}" ]; then
        export GIT_BRANCH_NAME="${CIRRUS_BRANCH}"
    else
        export GIT_BRANCH_NAME=master
    fi
fi

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
