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

project(PlatformUtils_cmio LANGUAGES CXX)

include(../../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/preferences.cpp
    src/preferences.h
    src/utils.cpp
    src/utils.h)

if (APPLE OR FAKE_APPLE)
    add_library(PlatformUtils_cmio STATIC ${SOURCES})
else ()
    add_library(PlatformUtils_cmio STATIC EXCLUDE_FROM_ALL ${SOURCES})
endif ()

add_dependencies(PlatformUtils_cmio VCamUtils)
target_compile_definitions(PlatformUtils_cmio PRIVATE PLATFORMUTILS_LIBRARY)
target_include_directories(PlatformUtils_cmio
                           PRIVATE
                           ..
                           ../..)

set(CMIO_ASSISTANT_NAME "io.github.webcamoid.${AKVCAM_PLUGIN_NAME}.${AKVCAM_SERVICE_NAME}")
add_definitions(-DCMIO_ASSISTANT_NAME="${CMIO_ASSISTANT_NAME}")

if (FAKE_APPLE)
    target_include_directories(PlatformUtils_cmio
                               PRIVATE
                               ../FakeAPI)
endif ()

add_definitions(-DBINDIR="${BINDIR}"
                -DDATAROOTDIR="${DATAROOTDIR}"
                -DAKVCAM_SERVICEPORT_DEFAULT="${AKVCAM_SERVICEPORT}")

if (FAKE_APPLE)
    add_definitions(-DFAKE_APPLE)
endif ()

if (APPLE OR FAKE_APPLE)
    if (APPLE)
        find_library(COREFOUNDATION_FRAMEWORK NAMES CoreFoundation)
        find_library(COREGRAPHICS_FRAMEWORK NAMES CoreGraphics)
        set(EXTRA_LIBS
            ${COREFOUNDATION_FRAMEWORK}
            ${COREGRAPHICS_FRAMEWORK})
    endif ()
endif ()

target_link_libraries(PlatformUtils_cmio
                      VCamUtils
                      ${EXTRA_LIBS})
