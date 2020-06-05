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

if "%PLATFORM%" == "x86" (
    set VC_ARGS=x86
) else (
    set VC_ARGS=amd64
)

rem Visual Studio init
if not "%VSPATH%" == "" call "%VSPATH%\vcvarsall" %VC_ARGS%

set PATH_ORIG=%PATH%
set PATH=%QTDIR%\bin;%TOOLSDIR%\bin;%PATH%

qmake -query
qmake akvirtualcamera.pro ^
    CONFIG+=%CONFIGURATION% ^
    CONFIG+=silent ^
    PREFIX="%INSTALL_PREFIX%"

%MAKETOOL% -j4

if "%DAILY_BUILD%" == "" goto EndScript

if "%PLATFORM%" == "x86" (
    set DRV_ARCH=x64
) else (
    set DRV_ARCH=x86
)

echo.
echo Building %DRV_ARCH% virtual camera driver
echo.

mkdir akvcam
cd akvcam

set PATH=%QTDIR_ALT%\bin;%TOOLSDIR_ALT%\bin;%PATH_ORIG%
qmake -query
qmake ^
    CONFIG+=silent ^
    ..\akvirtualcamera.pro
%MAKETOOL% -j4

cd ..
mkdir AkVirtualCamera.plugin\%DRV_ARCH%
xcopy ^
    akvcam\AkVirtualCamera.plugin\%DRV_ARCH%\* ^
    AkVirtualCamera.plugin\%DRV_ARCH% ^
    /i /y

:EndScript
