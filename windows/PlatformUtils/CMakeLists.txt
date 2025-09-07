# akvirtualcamera, virtual camera for Mac and Windows.
# Copyright (C) 2021  Gonzalo Exequiel Pedone
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

cmake_minimum_required(VERSION 3.16)

project(PlatformUtils_windows LANGUAGES CXX)

include(../../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/cunknown.h
    src/cunknown.cpp
    src/preferences.cpp
    src/preferences.h
    src/utils.cpp
    src/utils.h
    src/videoprocamp.cpp
    src/videoprocamp.h)

if (WIN32)
    add_library(PlatformUtils_windows STATIC ${SOURCES})
else ()
    add_library(PlatformUtils_windows STATIC EXCLUDE_FROM_ALL ${SOURCES})
endif ()

add_dependencies(PlatformUtils_windows VCamUtils)
target_compile_definitions(PlatformUtils_windows PRIVATE PLATFORMUTILS_LIBRARY)
target_include_directories(PlatformUtils_windows PRIVATE ../..)

add_definitions(-DNOMINMAX
                -DAKVCAM_SERVICEPORT_DEFAULT="${AKVCAM_SERVICEPORT}")

if (WIN32)
    set(EXTRA_LIBS
        advapi32
        kernel32
        gdi32
        shell32
        strmiids
        windowscodecs)
endif ()

target_link_libraries(PlatformUtils_windows
                      VCamUtils
                      ${EXTRA_LIBS})
