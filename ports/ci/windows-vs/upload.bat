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

set SOURCES_DIR=%cd%
set PACKAGES_DIR=%SOURCES_DIR%\webcamoid-packages

if not "%USE_WGET%" == "" (
    set DOWNLOAD_CMD="wget -nv -c"
) else (
    set DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
)

set upload=
if not "%DAILY_BUILD%" == "" set upload=true
if not "%RELEASE_BUILD%" == "" set upluad=true
if "%upload%" == "true" (
    REM Upload to Github Releases
    set hub=hub-windows-amd64-%GITHUB_HUBVER%

    cd "%SOURCES_DIR%"
    %DOWNLOAD_CMD% "https://github.com/github/hub/releases/download/v%GITHUB_HUBVER%/%hub%.zip"
    unzip "%hub%.zip"
    mkdir .local
    xcopy "%hub%"\* .local /i /y

    set PATH=%cd%/.local/bin;%PATH%
    set path=webcamoid-packages

    for %%f in (%path%\*) do (
        hub release edit -m "Daily Build" -a %%f daily-build
    )
)
