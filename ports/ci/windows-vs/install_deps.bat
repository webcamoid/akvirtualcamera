REM akvirtualcamera, virtual camera for Mac and Windows.
REM Copyright (C) 2021  Gonzalo Exequiel Pedone
REM
REM akvirtualcamera is free software: you can redistribute it and/or modify
REM it under the terms of the GNU General Public License as published by
REM the Free Software Foundation, either version 3 of the License, or
REM (at your option) any later version.
REM
REM akvirtualcamera is distributed in the hope that it will be useful,
REM but WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM GNU General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with akvirtualcamera. If not, see <http://www.gnu.org/licenses/>.
REM
REM Web-Site: http://webcamoid.github.io/

@echo off

set "DOWNLOAD_CMD=curl --retry 10 -sS -k -L -O -C -"

if "%NSIS_VERSION%" == "" set NSIS_VERSION=3.10
set "MAJOR_VERSION=%NSIS_VERSION:~0,1%"
set "nsis=nsis-%NSIS_VERSION%-setup.exe"
set "URL=https://sourceforge.net/projects/nsis/files/NSIS%%20%MAJOR_VERSION%/%NSIS_VERSION%/%nsis%/download"

echo Downloading %URL%
%DOWNLOAD_CMD% "%URL%"

if exist download (
    ren download %nsis%
    "%nsis%" /S
)
