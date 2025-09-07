# akvirtualcamera, virtual camera for Mac and Windows.
# Copyright (C) 2025  Gonzalo Exequiel Pedone
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

project(VCamMediaSource LANGUAGES CXX)

include(../../../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/mfvcam.h
    src/mediaeventgenerator.cpp
    src/mediaeventgenerator.h
    src/mediasource.cpp
    src/mediasource.h
    src/mediastream.cpp
    src/mediastream.h)

if (WIN32)
    add_library(VCamMediaSource STATIC ${SOURCES})
else ()
    add_library(VCamMediaSource STATIC EXCLUDE_FROM_ALL ${SOURCES})
endif ()

add_dependencies(VCamMediaSource VCamIPC_windows PlatformUtils_windows VCamUtils)
target_compile_definitions(VCamMediaSource PRIVATE VCAMMEDIASOURCE_LIBRARY)
target_include_directories(VCamMediaSource
                           PRIVATE
                           ..
                           ../..
                           ../../..)

target_link_libraries(VCamMediaSource
                      VCamIPC_windows
                      PlatformUtils_windows
                      VCamUtils)
add_definitions(-DNOMINMAX
                -DSTRSAFE_NO_DEPRECATE)
