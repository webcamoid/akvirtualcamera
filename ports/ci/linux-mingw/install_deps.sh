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

if [ ! -z "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'

# Download chroot image
archImage=archlinux-bootstrap-${ARCH_ROOT_DATE}-x86_64.tar.gz
${DOWNLOAD_CMD} "${ARCH_ROOT_URL}/iso/${ARCH_ROOT_DATE}/$archImage"
sudo tar xzf "$archImage"

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
sudo mkdir -pv "root.x86_64/$HOME"
sudo mount --bind root.x86_64 root.x86_64
sudo mount --bind "$HOME" "root.x86_64/$HOME"

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
    mingw-w64-cmake

    # Install NSIS

nsis=nsis-${NSIS_VERSION}-setup.exe
${DOWNLOAD_CMD} "https://sourceforge.net/projects/nsis/files/NSIS%20${NSIS_VERSION:0:1}/${NSIS_VERSION}/${nsis}"

if [ -e "${nsis}" ]; then
    INSTALLSCRIPT=installscript.sh

    cat << EOF > ${INSTALLSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
export WINEPREFIX=/opt/.wine
cd "${SOURCES_DIR}"

wine ./${nsis} /S
EOF

    chmod +x ${INSTALLSCRIPT}
    sudo cp -vf ${INSTALLSCRIPT} "root.x86_64/$HOME/"
    ${EXEC} bash "$HOME/${INSTALLSCRIPT}"
fi

# Finish

sudo umount -f "root.x86_64/$HOME" || true
sudo umount -f root.x86_64 || true
