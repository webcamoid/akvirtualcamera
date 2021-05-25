#!/bin/bash

# Webcamoid, webcam capture application.
# Copyright (C) 2017  Gonzalo Exequiel Pedone
#
# Webcamoid is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamoid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
#
# Web-Site: http://webcamoid.github.io/

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
fi

git clone https://github.com/webcamoid/DeployTools.git

DEPLOYSCRIPT=deployscript.sh

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind "$HOME" "root.x86_64/$HOME"

    cat << EOF > package_info_strip.conf
[System]
stripCmd = x86_64-w64-mingw32-strip
EOF

    cat << EOF > ${DEPLOYSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=${HOME}
export PATH="${TRAVIS_BUILD_DIR}/.local/bin:\$PATH"
export INSTALL_PREFIX="${TRAVIS_BUILD_DIR}/webcamoid-data"
export PACKAGES_DIR="${TRAVIS_BUILD_DIR}/webcamoid-packages/windows"
export PYTHONPATH="${TRAVIS_BUILD_DIR}/DeployTools"
export BUILD_PATH="${TRAVIS_BUILD_DIR}/build-x64"
export WINEPREFIX=/opt/.wine
cd $TRAVIS_BUILD_DIR
EOF

    if [ ! -z "${DAILY_BUILD}" ]; then
        cat << EOF >> ${DEPLOYSCRIPT}
export DAILY_BUILD=1
EOF
    fi

    cat << EOF >> ${DEPLOYSCRIPT}
cd build-x64
cmake --build . --target install
cd ..
cd build-x86
cmake --build . --target install
cd ..

i686-w64-mingw32-strip \${INSTALL_PREFIX}/x86/*
x86_64-w64-mingw32-strip \${INSTALL_PREFIX}/x64/*

python ./DeployTools/deploy.py \
        -d "\${INSTALL_PREFIX}" \
        -c "\${BUILD_PATH}/package_info.conf" \
        -c "./package_info_strip.conf" \
        -o "\${PACKAGES_DIR}"
EOF
    chmod +x ${DEPLOYSCRIPT}
    sudo cp -vf ${DEPLOYSCRIPT} "root.x86_64/$HOME/"

    ${EXEC} bash "$HOME/${DEPLOYSCRIPT}"
    sudo umount "root.x86_64/$HOME"
    sudo umount root.x86_64
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    cd build
    cmake --build . --target install
    cd ..

    export INSTALL_PREFIX="${PWD}/webcamoid-data"
    export PACKAGES_DIR="${PWD}/webcamoid-packages/mac"
    export PYTHONPATH="${PWD}/DeployTools"
    export BUILD_PATH="${PWD}/build"
    python3 ./DeployTools/deploy.py \
        -d "${INSTALL_PREFIX}" \
        -c "${BUILD_PATH}/package_info.conf" \
        -o "${PACKAGES_DIR}"
fi
