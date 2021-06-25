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

REM set upload=
REM if not "%DAILY_BUILD%" == "" set upload=true
REM if not "%RELEASE_BUILD%" == "" set upluad=true
REM if "%upload%" == "true" (
REM     REM Upload to Github Releases
REM     set hub=hub-windows-amd64-%GITHUB_HUBVER%
REM
REM     cd "%SOURCES_DIR%"
REM     %DOWNLOAD_CMD% "https://github.com/github/hub/releases/download/v%GITHUB_HUBVER%/%hub%.zip"
REM     unzip "%hub%.zip"
REM     mkdir .local
REM     xcopy "%hub%"\* .local /i /y
REM
REM     set PATH=%cd%/.local/bin;%PATH%
REM     set path=webcamoid-packages
REM
REM     for %%f in (%path%\*) do (
REM         hub release edit -m "Daily Build" -a %%f daily-build
REM     )
REM )
