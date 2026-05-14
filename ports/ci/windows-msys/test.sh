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

if [ "${COMPILER}" = clang ]; then
    COMPILER_C=clang
    COMPILER_CXX=clang++
else
    COMPILER_C=gcc
    COMPILER_CXX=g++
fi

echo "Starting the FrameServer"

cmd //c "sc start FrameServer" || true
cmd //c "sc start FrameServerMonitor" || true

export BUILD_DIR="${PWD}/build-${COMPILER}-x64/build"
export INSTALL_PREFIX="C:/Program Files/AkVirtualCamera"

mkdir -p "${INSTALL_PREFIX}"
cp -rvf "${BUILD_DIR}"/* "${INSTALL_PREFIX}/"

echo "Initilize the assistant"
nohup "${INSTALL_PREFIX}/x64/AkVCamAssistant.exe" &

# Configure a virtual camera for testing

manager="${INSTALL_PREFIX}/x64/AkVCamManager.exe"

"${manager}" add-device -i FakeCamera0 "Virtual Camera"
"${manager}" add-format FakeCamera0 RGB24 640 480 30
"${manager}" add-format FakeCamera0 RGB32 640 480 30
"${manager}" update
"${manager}" devices
"${manager}" formats FakeCamera0
"${manager}" set-loglevel 7

echo "Initilize the Media Foundation assistant"
nohup "${INSTALL_PREFIX}/x64/AkVCamAssistantMF.exe" &
sleep 20

echo "Testing the virtual camera in DirectShow"

"${COMPILER_CXX}" -o testds.exe ports/ci/windows-msys/testds.cpp -lole32 -loleaut32 -lstrmiids -lquartz
./testds.exe

echo "Testing the virtual camera in Media Foundation"

"${COMPILER_CXX}" -o testmf.exe ports/ci/windows-msys/testmf.cpp -lole32 -lmf -lmfplat -lmfreadwrite -lmfuuid
./testmf.exe

echo "Checking if the services are up"

pgrep -a AkVCam || true

echo "Log"

cat /d/a/_temp/msys64/tmp/AkVirtualCameraMF-*.log || true

echo "Log (Temp)"

cat /c/Windows/Temp/*.log || true
