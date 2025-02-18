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

project(VCamIPC_cmio LANGUAGES CXX)
project(VCamIPC_cmio_shared LANGUAGES CXX)

include(../../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/ipcbridge.cpp)

if (APPLE OR FAKE_APPLE)
    add_library(VCamIPC_cmio STATIC ${SOURCES})
    add_library(VCamIPC_cmio_shared SHARED ${SOURCES})
    set_target_properties(VCamIPC_cmio_shared PROPERTIES
                          OUTPUT_NAME ${AKVCAM_BRIDGE_NAME}
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${LIBDIR}
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR}
                          PREFIX "")
else ()
    add_library(VCamIPC_cmio STATIC EXCLUDE_FROM_ALL ${SOURCES})
    add_library(VCamIPC_cmio_shared SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

add_dependencies(VCamIPC_cmio VCamUtils)
add_dependencies(VCamIPC_cmio_shared VCamUtils)

if (APPLE)
    add_dependencies(VCamIPC_cmio PlatformUtilsObjC_cmio)
    add_dependencies(VCamIPC_cmio_shared PlatformUtilsObjC_cmio)
endif ()

target_compile_definitions(VCamIPC_cmio PRIVATE VCAMIPC_LIBRARY)
target_compile_definitions(VCamIPC_cmio_shared PRIVATE VCAMIPC_LIBRARY_SHARED)
target_include_directories(VCamIPC_cmio
                           PRIVATE
                           ..
                           ../..)
target_include_directories(VCamIPC_cmio_shared
                           PRIVATE
                           ..
                           ../..)

if (FAKE_APPLE)
    target_include_directories(VCamIPC_cmio
                               PRIVATE
                               ../FakeAPI)
    target_include_directories(VCamIPC_cmio_shared
                               PRIVATE
                               ../FakeAPI)
endif ()

if (APPLE OR FAKE_APPLE)
    if (APPLE)
        find_library(FOUNDATION_FRAMEWORK NAMES Foundation)
        set(EXTRA_LIBS ${FOUNDATION_FRAMEWORK})
    endif ()
endif ()

if (APPLE)
    set(EXTRA_OBJC PlatformUtilsObjC_cmio)
endif ()

target_link_libraries(VCamIPC_cmio
                      VCamUtils
                      ${EXTRA_LIBS}
                      ${EXTRA_OBJC})
target_link_libraries(VCamIPC_cmio_shared
                      VCamUtils
                      ${EXTRA_LIBS}
                      ${EXTRA_OBJC})

if (FAKE_APPLE)
    add_definitions(-DFAKE_APPLE)
endif ()
    add_definitions(-DFAKE_APPLE)

if (APPLE OR FAKE_APPLE)
    install(TARGETS VCamIPC_cmio_shared DESTINATION ${LIBDIR})
endif ()
