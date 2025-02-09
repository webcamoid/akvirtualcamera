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

cmake_minimum_required(VERSION 3.14)

project(PlatformUtils_dshow LANGUAGES CXX)

include(../../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/preferences.cpp
    src/preferences.h
    src/utils.cpp
    src/utils.h)

if (WIN32)
    add_library(PlatformUtils_dshow STATIC ${SOURCES})
else ()
    add_library(PlatformUtils_dshow STATIC EXCLUDE_FROM_ALL ${SOURCES})
endif ()

add_dependencies(PlatformUtils_dshow VCamUtils)
target_compile_definitions(PlatformUtils_dshow PRIVATE PLATFORMUTILS_LIBRARY)
target_include_directories(PlatformUtils_dshow PRIVATE ../..)

add_definitions(-DAKVCAM_SERVICEPORT_DEFAULT="${AKVCAM_SERVICEPORT}")

if (WIN32)
    set(EXTRA_LIBS
        advapi32
        kernel32
        gdi32
        shell32
        strmiids
        windowscodecs)
endif ()

target_link_libraries(PlatformUtils_dshow
                      VCamUtils
                      ${EXTRA_LIBS})
