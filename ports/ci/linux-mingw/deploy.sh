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
EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'

git clone https://github.com/webcamoid/DeployTools.git

DEPLOYSCRIPT=deployscript.sh

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
export PATH="${SOURCES_DIR}/.local/bin:\$PATH"
export INSTALL_PREFIX="${SOURCES_DIR}/webcamoid-data"
export PACKAGES_DIR="${SOURCES_DIR}/webcamoid-packages"
export PYTHONPATH="${SOURCES_DIR}/DeployTools"
export BUILD_PATH="${SOURCES_DIR}/build-x64"
export WINEPREFIX=/opt/.wine
cd "${SOURCES_DIR}"
EOF

if [ ! -z "${DAILY_BUILD}" ]; then
    cat << EOF >> ${DEPLOYSCRIPT}
export DAILY_BUILD=1
EOF
fi

cat << EOF >> ${DEPLOYSCRIPT}
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
