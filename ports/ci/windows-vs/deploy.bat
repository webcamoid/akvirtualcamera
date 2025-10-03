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

if not "%GITHUB_SHA%" == "" set GIT_COMMIT_HASH="%GITHUB_SHA%"
if not "%CIRRUS_CHANGE_IN_REPO%" == "" set GIT_COMMIT_HASH="%CIRRUS_CHANGE_IN_REPO%"

if not "%GITHUB_REF_NAME%" == "" set GIT_BRANCH_NAME="%GITHUB_REF_NAME%"
if not "%CIRRUS_BRANCH%" == "" set GIT_BRANCH_NAME="%CIRRUS_BRANCH%"
if "%GIT_BRANCH_NAME%" == "" set GIT_BRANCH_NAME=master

git clone https://github.com/webcamoid/DeployTools.git

set PATH="C:\Program Files (x86)\NSIS;%Python_ROOT_DIR%;%PATH%"
set INSTALL_PREFIX=%CD%\package-data
set PACKAGES_DIR=%CD%\packages\windows
set BUILD_PATH=%CD%\build-x64
set PYTHONPATH=%CD%\DeployTools

python DeployTools\deploy.py ^
    -d "%INSTALL_PREFIX%" ^
    -c "%BUILD_PATH%\package_info.conf" ^
    -o "%PACKAGES_DIR%"
