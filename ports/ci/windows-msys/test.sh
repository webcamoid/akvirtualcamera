#!/bin/bash

# akvirtualcamera, virtual camera for Mac and Windows.
# Copyright (C) 2025  Gonzalo Exequiel Pedone
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

export INSTALL_PREFIX="${PWD}/package-data-${COMPILER}"

# configure a virtual camera for testing

manager="${INSTALL_PREFIX}/x64/AkVCamManager.exe"

"${manager}" add-device -i FakeCamera0 "Virtual Camera"
"${manager}" add-format FakeCamera0 RGB24 640 480 30
"${manager}" update
"${manager}" devices
"${manager}" formats FakeCamera0

# Compile the testing program and execute it

"${COMPILER}" -o test.exe ports/ci/windows-msys/test.cpp -lole32 -lmf -lmfplat -lmfuuid
./test.exe
