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

rem Installing various utilities
choco install -y jfrog-cli
setlocal

if "%CMAKE_GENERATOR%" == "MSYS Makefiles" set PATH=C:\msys64\usr\bin;%PATH%
if "%CMAKE_GENERATOR%" == "MSYS Makefiles" (
    bash -lc "cd /c/projects/akvirtualcamera && ./ports/ci/appveyor/install_deps.sh"
)

endlocal
