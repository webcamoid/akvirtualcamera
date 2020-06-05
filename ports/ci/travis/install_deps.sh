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

#qtIinstallerVerbose=-v

if [ ! -z "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

if [ "${TRAVIS_OS_NAME}" = linux ] && [ "${ANDROID_BUILD}" != 1 ]; then
    if [ "${ARCH_ROOT_BUILD}" = 1 ]; then
        EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
    else
        EXEC="docker exec ${DOCKERSYS}"
    fi
fi

if [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    # Download chroot image
    archImage=archlinux-bootstrap-${ARCH_ROOT_DATE}-x86_64.tar.gz
    ${DOWNLOAD_CMD} ${ARCH_ROOT_URL}/iso/${ARCH_ROOT_DATE}/$archImage
    sudo tar xzf $archImage

    # Configure mirrors
    cp -vf root.x86_64/etc/pacman.conf .
    cat << EOF >> pacman.conf

[multilib]
Include = /etc/pacman.d/mirrorlist

[ownstuff]
Server = https://ftp.f3l.de/~martchus/\$repo/os/\$arch
Server = http://martchus.no-ip.biz/repo/arch/\$repo/os/\$arch
EOF
    sed -i 's/Required DatabaseOptional/Never/g' pacman.conf
    sed -i 's/#TotalDownload/TotalDownload/g' pacman.conf
    sudo cp -vf pacman.conf root.x86_64/etc/pacman.conf

    cp -vf root.x86_64/etc/pacman.d/mirrorlist .
    cat << EOF >> mirrorlist

Server = ${ARCH_ROOT_URL}/\$repo/os/\$arch
EOF
    sudo cp -vf mirrorlist root.x86_64/etc/pacman.d/mirrorlist

    # Install packages
    sudo mkdir -pv root.x86_64/$HOME
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind $HOME root.x86_64/$HOME

    ${EXEC} pacman-key --init
    ${EXEC} pacman-key --populate archlinux
    ${EXEC} pacman -Syu \
        --noconfirm \
        --ignore linux,linux-api-headers,linux-docs,linux-firmware,linux-headers,pacman

    ${EXEC} pacman --noconfirm --needed -S \
        ccache \
        clang \
        file \
        git \
        make \
        pkgconf \
        python \
        sed \
        xorg-server-xvfb \
        wine \
        mingw-w64-pkg-config \
        mingw-w64-gcc \
        mingw-w64-qt5-base

    for mingw_arch in i686 x86_64; do
        if [ "$mingw_arch" = x86_64 ]; then
            ff_arch=win64
        else
            ff_arch=win32
        fi

    qtIFW=QtInstallerFramework-win-x86.exe

    # Install Qt Installer Framework
    ${DOWNLOAD_CMD} http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/${qtIFW} || true

    if [ -e ${qtIFW} ]; then
        INSTALLSCRIPT=installscript.sh

        cat << EOF > ${INSTALLSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
export WINEPREFIX=/opt/.wine
cd $TRAVIS_BUILD_DIR

wine ./${qtIFW} \
    ${qtIinstallerVerbose} \
    --script "ports/ci/travis/qtifw_non_interactive_install.qs" \
    --no-force-installations
EOF

        chmod +x ${INSTALLSCRIPT}
        sudo cp -vf ${INSTALLSCRIPT} root.x86_64/$HOME/
        ${EXEC} bash $HOME/${INSTALLSCRIPT}
    fi

    # Finish
    sudo umount root.x86_64/$HOME
    sudo umount root.x86_64
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    brew update
    brew upgrade
    brew link --overwrite numpy
    brew install \
        p7zip \
        python \
        ccache \
        pkg-config \
        qt5 \
    brew link --overwrite python

    brew link python
    qtIFW=QtInstallerFramework-mac-x64.dmg

    # Install Qt Installer Framework
    ${DOWNLOAD_CMD} http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/${qtIFW} || true

    if [ -e "${qtIFW}" ]; then
        hdiutil convert ${qtIFW} -format UDZO -o qtifw
        7z x -oqtifw qtifw.dmg -bb
        7z x -oqtifw qtifw/5.hfsx -bb
        chmod +x qtifw/QtInstallerFramework-mac-x64/QtInstallerFramework-mac-x64.app/Contents/MacOS/QtInstallerFramework-mac-x64

        qtifw/QtInstallerFramework-mac-x64/QtInstallerFramework-mac-x64.app/Contents/MacOS/QtInstallerFramework-mac-x64 \
            ${qtIinstallerVerbose} \
            --script "$PWD/ports/ci/travis/qtifw_non_interactive_install.qs" \
            --no-force-installations
    fi
fi
