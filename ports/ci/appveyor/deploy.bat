REM Webcamoid, webcam capture application.
REM Copyright (C) 2017  Gonzalo Exequiel Pedone
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
    set PYTHON_PATH=C:\%PYTHON_VERSION%
) else (
    set PYTHON_PATH=C:\%PYTHON_VERSION%-x64
)

cd ports/deploy
git clone https://github.com/webcamoid/DeployTools.git
cd ../..

set PYTHONPATH=%cd%\ports\deploy\DeployTools

setlocal
if "%CMAKE_GENERATOR%" == "MSYS Makefiles" set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%
cd build-x64
cmake --build . --target install 
cd ..
cd build-x86
cmake --build . --target install 
cd ..
endlocal

%PYTHON_PATH%\python.exe ports\deploy\deploy.py
