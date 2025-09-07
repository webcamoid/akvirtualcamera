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

project(VirtualCameraMFService LANGUAGES CXX)

include(../../../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    add_executable(VirtualCameraMFService src/main.cpp)
else ()
    add_executable(VirtualCameraMFService EXCLUDE_FROM_ALL src/main.cpp)
endif ()

set_target_properties(VirtualCameraMFService PROPERTIES
                      OUTPUT_NAME ${AKVCAM_SERVICE_MF_NAME}
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR})
add_dependencies(VirtualCameraMFService VCamMediaSource PlatformUtils_windows MFUtils)
target_compile_definitions(VirtualCameraMFService PRIVATE VIRTUALCAMERAMF_SERVICE)
target_include_directories(VirtualCameraMFService
                           PRIVATE
                           ../MediaSource/src
                           ..
                           ../..
                           ../../..)

if (WIN32)
    set(EXTRA_LIBS
        mfplat
        mfuuid)
endif ()

target_link_libraries(VirtualCameraMFService
                      VCamMediaSource
                      PlatformUtils_windows
                      MFUtils
                      ${EXTRA_LIBS})

add_definitions(-DNOMINMAX
                -DSTRSAFE_NO_DEPRECATE)

if (WIN32)
    install(TARGETS VirtualCameraMFService DESTINATION ${BINDIR})
endif ()
