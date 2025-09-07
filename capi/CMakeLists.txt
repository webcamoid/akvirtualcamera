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

project(VCam_capi LANGUAGES CXX)

include(../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/capi.h
    src/capi.cpp)

add_library(VCam_capi SHARED ${SOURCES})

if (APPLE OR FAKE_APPLE)
    set_target_properties(VCam_capi PROPERTIES
                          OUTPUT_NAME vcam_capi
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${LIBDIR})
else ()
    set_target_properties(VCam_capi PROPERTIES
                          OUTPUT_NAME vcam_capi
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR}
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR})
endif ()

add_dependencies(VCam_capi VCamUtils)

if (APPLE OR FAKE_APPLE)
    add_dependencies(VCam_capi
                     VCamIPC_cmio
                     PlatformUtils_cmio)
elseif (WIN32)
    add_dependencies(VCam_capi
                     VCamIPC_windows
                     PlatformUtils_windows)
    add_definitions(-DNOMINMAX)
endif ()

target_compile_definitions(VCam_capi PRIVATE CAPI_LIBRARY)
target_include_directories(VCam_capi
                           PRIVATE ..)
target_link_libraries(VCam_capi
                      VCamUtils)

if (APPLE OR FAKE_APPLE)
    target_include_directories(VCam_capi
                               PRIVATE
                               ../cmio)

    if (FAKE_APPLE)
        target_include_directories(VCam_capi
                                   PRIVATE
                                   ../cmio/FakeAPI)
    endif ()

    target_link_libraries(VCam_capi
                          VCamIPC_cmio
                          PlatformUtils_cmio)
elseif (WIN32)
    target_include_directories(VCam_capi
                               PRIVATE
                               ../windows)
    target_link_libraries(VCam_capi
                          VCamIPC_windows
                          PlatformUtils_windows)
endif ()

if (APPLE OR FAKE_APPLE)
    install(TARGETS VCam_capi DESTINATION ${LIBDIR})
elseif (WIN32)
    install(TARGETS VCam_capi DESTINATION ${BINDIR})
endif ()
