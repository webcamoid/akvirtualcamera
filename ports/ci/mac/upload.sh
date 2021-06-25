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

if [[ ! -z "$DAILY_BUILD" || ! -z "$RELEASE_BUILD" ]]; then
    # Upload to Github Releases
    hub=hub-darwin-amd64-${GITHUB_HUBVER}

    cd "${SOURCES_DIR}"
    ${DOWNLOAD_CMD} "https://github.com/github/hub/releases/download/v${GITHUB_HUBVER}/${hub}.tgz" || true
    tar xzf "${hub}.tgz"
    mkdir -p .local
    cp -rf "${hub}"/* .local/

    export PATH="${PWD}/.local/bin:${PATH}"
    path=webcamoid-packages

    for f in $(find $path -type f); do
        hub release edit -m 'Daily Build' -a "$f" daily-build
    done
fi
