REM Webcamoid, webcam capture application.
REM Copyright (C) 2016  Gonzalo Exequiel Pedone
REM
REM Webcamoid is free software: you can redistribute it and/or modify
REM it under the terms of the GNU General Public License as published by
REM the Free Software Foundation, either version 3 of the License, or
REM (at your option) any later version.
REM
REM Webcamoid is distributed in the hope that it will be useful,
REM but WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM GNU General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
REM
REM Web-Site: http://webcamoid.github.io/

echo.
echo Building x64 virtual camera driver
echo.

set PATH_ORIG=%PATH%
mkdir build-x64
cd build-x64

if "%CMAKE_GENERATOR%" == "MSYS Makefiles" (
    set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;"%PATH%"
    echo %PATH%
    cmake ^
        -G "%CMAKE_GENERATOR%" ^
        -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" ^
        ..
)

if "%CMAKE_GENERATOR:~0,13%" == "Visual Studio" (
    set PATH="C:\Program Files\CMake\bin;%PATH%"
    echo %PATH%
    cmake ^
        -G "%CMAKE_GENERATOR%" ^
        -A x64 ^
        -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" ^
        ..
)

cmake --build .
cd ..

echo.
echo Building x86 virtual camera driver
echo.

set PATH=%PATH_ORIG%
mkdir build-x86
cd build-x86

if "%CMAKE_GENERATOR%" == "MSYS Makefiles" (
    set PATH=C:\msys64\mingw32\bin;C:\msys64\usr\bin;"%PATH%"
    cmake ^
        -G "%CMAKE_GENERATOR%" ^
        -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" ^
        ..
)

if "%CMAKE_GENERATOR:~0,13%" == "Visual Studio" (
    set PATH="C:\Program Files\CMake\bin;%PATH%"
    cmake ^
        -G "%CMAKE_GENERATOR%" ^
        -A Win32 ^
        -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" ^
        ..
)

cmake --build .
cd ..

mkdir build-x64\AkVirtualCamera.plugin\x86
xcopy ^
    build-x86\AkVirtualCamera.plugin\x86\* ^
    build-x64\AkVirtualCamera.plugin\x86 ^
    /i /y
