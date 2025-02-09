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

if (APPLE)
    project(VCamIPC_cmio LANGUAGES CXX OBJCXX)
else ()
    project(VCamIPC_cmio LANGUAGES CXX)
endif ()

include(../../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/hacks.mm
    src/hacks.h)

if (APPLE)
    add_library(PlatformUtilsObjC_cmio STATIC ${SOURCES})
else ()
    add_library(PlatformUtilsObjC_cmio STATIC EXCLUDE_FROM_ALL ${SOURCES})
endif ()

add_dependencies(PlatformUtilsObjC_cmio PlatformUtils_cmio VCamUtils)
target_compile_definitions(PlatformUtilsObjC_cmio PRIVATE PLATFORMUTILSOBJC_LIBRARY)
target_include_directories(PlatformUtilsObjC_cmio PRIVATE ../..)

if (APPLE)
    find_library(COREFOUNDATION_FRAMEWORK NAMES CoreFoundation)
    set(EXTRA_LIBS
        ${COREFOUNDATION_FRAMEWORK})
endif ()

target_link_libraries(PlatformUtilsObjC_cmio
                      PlatformUtils_cmio
                      VCamUtils
                      ${EXTRA_LIBS})
