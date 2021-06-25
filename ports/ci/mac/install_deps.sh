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

if [ ! -z "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

brew update
brew upgrade
brew link --overwrite numpy
brew install \
    ccache \
    cmake \
    p7zip \
    pkg-config \
    python
brew link --overwrite python
brew link python

# Install Qt Installer Framework
qtIFW=QtInstallerFramework-macOS-x86_64-${QTIFWVER}.dmg
${DOWNLOAD_CMD} "http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/${qtIFW}" || true

if [ -e "${qtIFW}" ]; then
    hdiutil convert "${qtIFW}" -format UDZO -o qtifw
    7z x -oqtifw qtifw.dmg -bb
    7z x -oqtifw qtifw/5.hfsx -bb
    installer=qtifw/QtInstallerFramework-macOS-x86_64-${QTIFWVER}/QtInstallerFramework-macOS-x86_64-${QTIFWVER}.app/Contents/MacOS/QtInstallerFramework-macOS-x86_64-${QTIFWVER}
    chmod +x "${installer}"
    ${installer} \
        --verbose \
        --accept-licenses \
        --accept-messages \
        --confirm-command \
        install
fi
