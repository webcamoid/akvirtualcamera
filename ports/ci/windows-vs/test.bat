rem akvirtualcamera, virtual camera for Mac and Windows.
rem Copyright (C) 2025  Gonzalo Exequiel Pedone
rem
rem akvirtualcamera is free software: you can redistribute it and/or modify
rem it under the terms of the GNU General Public License as published by
rem the Free Software Foundation, either version 3 of the License, or
rem (at your option) any later version.
rem
rem akvirtualcamera is distributed in the hope that it will be useful,
rem but WITHOUT ANY WARRANTY; without even the implied warranty of
rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
rem GNU General Public License for more details.
rem
rem You should have received a copy of the GNU General Public License
rem along with akvirtualcamera. If not, see <http://www.gnu.org/licenses/>.
rem
rem Web-Site: http://webcamoid.github.io/

@echo off
setlocal enabledelayedexpansion

echo Configure th MSVC environment for x64

call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64

set INSTALL_PREFIX=%CD%\build-x64\build
echo Initialize the assistant
start /b "" "%INSTALL_PREFIX%\x64\Release\AkVCamAssistant.exe"

echo Configuring a virtual camera for testing

set manager=%INSTALL_PREFIX%\x64\Release\AkVCamManager.exe
"%manager%" add-device -i FakeCamera0 "Virtual Camera"
"%manager%" add-format FakeCamera0 RGB24 640 480 30
"%manager%" update
"%manager%" devices
"%manager%" formats FakeCamera0
"%manager%" set-loglevel 7

echo Initialize the Media Foundation assistant
start /b "" "%INSTALL_PREFIX%\x64\Release\AkVCamAssistantMF.exe"

REM REM nohup gdb -batch ^
REM REM -ex 'set pagination off' ^
REM REM -ex 'handle SIGSEGV stop' ^
REM REM -ex 'run' ^
REM REM -ex 'bt full' ^
REM REM -ex 'info registers' ^
REM REM -ex 'quit' ^
REM REM --args "%INSTALL_PREFIX%\x64\Release\AkVCamAssistantMF.exe" ^> gdb_output.log 2^>^&1

timeout /t 20 /nobreak >nul

echo Testing the virtual camera in DirectShow
cl /Fe:testds.exe ports/ci/windows-msys/testds.cpp /link ole32.lib oleaut32.lib strmiids.lib
testds.exe

echo Testing the virtual camera in Media Foundation
cl /Fe:testmf.exe ports/ci/windows-msys/testmf.cpp /link ole32.lib mf.lib mfplat.lib mfuuid.lib
testmf.exe

echo Checking if the services are up
tasklist /fi "imagename eq AkVCam*" /fo csv 2>nul | findstr /i "AkVCam" || echo No AkVCam processes found

echo Log
if exist gdb_output.log (
    type gdb_output.log
)

for /f "delims=" %%f in ('dir /b /s "D:\a\_temp\msys64\tmp\AkVirtualCameraMF-*.log" 2^>nul') do (
    echo Contents of %%f:
    type "%%f"
)

endlocal
