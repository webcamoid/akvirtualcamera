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

BUILDSCRIPT=dockerbuild.sh

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind $HOME root.x86_64/$HOME
    cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
cd $TRAVIS_BUILD_DIR
echo
echo "Building x64 virtual camera driver"
echo
mkdir build-x64
cd build-x64
x86_64-w64-mingw32-cmake ..
cmake --build .
cd ..
echo
echo "Building x86 virtual camera driver"
echo
mkdir build-x86
cd build-x86
i686-w64-mingw32-cmake ..
cmake --build .
EOF
    chmod +x ${BUILDSCRIPT}
    sudo cp -vf ${BUILDSCRIPT} root.x86_64/$HOME/

    EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
    ${EXEC} bash $HOME/${BUILDSCRIPT}
    
    sudo mkdir -p build-x64/AkVirtualCamera.plugin/x86
    sudo cp -rvf \
        build-x86/AkVirtualCamera.plugin/x86/* \
        build-x64/AkVirtualCamera.plugin/x86/

    sudo umount root.x86_64/$HOME
    sudo umount root.x86_64
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    mkdir build
    cd build
    cmake ..
    cmake --build .
fi
