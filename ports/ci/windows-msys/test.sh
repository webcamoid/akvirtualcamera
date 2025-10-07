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

echo "Initilize the assistant"
nohup "${INSTALL_PREFIX}/x64/AkVCamAssistant.exe" &

# configure a virtual camera for testing

manager="${INSTALL_PREFIX}/x64/AkVCamManager.exe"

"${manager}" add-device -i FakeCamera0 "Virtual Camera"
"${manager}" add-format FakeCamera0 RGB24 640 480 30
"${manager}" update
"${manager}" devices
"${manager}" formats FakeCamera0

echo "Initilize the Media Foundation assistant"
#nohup "${INSTALL_PREFIX}/x64/AkVCamAssistantMF.exe" &
nohup gdb -ex=r --args "${INSTALL_PREFIX}/x64/AkVCamAssistantMF.exe" &

echo "Testing the virtual camera in DirectShow"

"${COMPILER}" -o testds.exe ports/ci/windows-msys/testds.cpp -lole32 -loleaut32 -lstrmiids
./testds.exe

echo "Testing the virtual camera in Media Foundation"

"${COMPILER}" -o testmf.exe ports/ci/windows-msys/testmf.cpp -lole32 -lmf -lmfplat -lmfuuid
./testmf.exe

echo "Checking if the services are up"

pgrep -a AkVCam || true
