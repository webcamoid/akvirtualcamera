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

if [ -z "${DISABLE_CCACHE}" ]; then
    if [ "${CXX}" = clang++ ]; then
        UNUSEDARGS="-Qunused-arguments"
    fi

    COMPILER="ccache ${CXX} ${UNUSEDARGS}"
else
    COMPILER=${CXX}
fi

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
fi

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind $HOME root.x86_64/$HOME

    QMAKE_CMD=/usr/${ARCH_ROOT_MINGW}-w64-mingw32/lib/qt/bin/qmake

    cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
EOF

    if [ ! -z "${DAILY_BUILD}" ]; then
        cat << EOF >> ${BUILDSCRIPT}
export DAILY_BUILD=1
EOF
    fi

    cat << EOF >> ${BUILDSCRIPT}
cd $TRAVIS_BUILD_DIR
${QMAKE_CMD} -query
${QMAKE_CMD} -spec ${COMPILESPEC} akvirtualcamera.pro \
    CONFIG+=silent \
    QMAKE_CXX="${COMPILER}"
EOF
    chmod +x ${BUILDSCRIPT}
    sudo cp -vf ${BUILDSCRIPT} root.x86_64/$HOME/

    ${EXEC} bash $HOME/${BUILDSCRIPT}
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    ${EXEC} qmake -query
    ${EXEC} qmake -spec ${COMPILESPEC} akvirtualcamera.pro \
        CONFIG+=silent \
        QMAKE_CXX="${COMPILER}"
fi

if [ -z "${NJOBS}" ]; then
    NJOBS=4
fi

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
EOF

    if [ ! -z "${DAILY_BUILD}" ]; then
        cat << EOF >> ${BUILDSCRIPT}
export DAILY_BUILD=1
EOF
    fi

    cat << EOF >> ${BUILDSCRIPT}
cd $TRAVIS_BUILD_DIR
make -j${NJOBS}
EOF
    chmod +x ${BUILDSCRIPT}
    sudo cp -vf ${BUILDSCRIPT} root.x86_64/$HOME/

    ${EXEC} bash $HOME/${BUILDSCRIPT}
    sudo umount root.x86_64/$HOME
    sudo umount root.x86_64
else
    ${EXEC} make -j${NJOBS}
fi

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    if [ "$ARCH_ROOT_MINGW" = x86_64 ]; then
        mingw_arch=i686
        mingw_compiler=${COMPILER/x86_64/i686}
        mingw_dstdir=x86
    else
        mingw_arch=x86_64
        mingw_compiler=${COMPILER/i686/x86_64}
        mingw_dstdir=x64
    fi

    echo
    echo "Building $mingw_arch virtual camera driver"
    echo
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind $HOME root.x86_64/$HOME

    cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
mkdir -p $TRAVIS_BUILD_DIR/akvcam
cd $TRAVIS_BUILD_DIR/akvcam
/usr/${mingw_arch}-w64-mingw32/lib/qt/bin/qmake \
    -spec ${COMPILESPEC} \
    ../akvirtualcamera.pro \
    CONFIG+=silent \
    QMAKE_CXX="${mingw_compiler}"
make -j${NJOBS}
EOF
    chmod +x ${BUILDSCRIPT}
    sudo cp -vf ${BUILDSCRIPT} root.x86_64/$HOME/

    ${EXEC} bash $HOME/${BUILDSCRIPT}

    sudo mkdir -p AkVirtualCamera.plugin/${mingw_dstdir}
    sudo cp -rvf \
        akvcam/AkVirtualCamera.plugin/${mingw_dstdir}/* \
        AkVirtualCamera.plugin/${mingw_dstdir}/

    sudo umount root.x86_64/$HOME
    sudo umount root.x86_64
fi
