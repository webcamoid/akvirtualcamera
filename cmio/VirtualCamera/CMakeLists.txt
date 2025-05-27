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

project(VirtualCamera_cmio LANGUAGES CXX)

include(../../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/clock.cpp
    src/clock.h
    src/device.cpp
    src/device.h
    src/object.cpp
    src/object.h
    src/objectinterface.cpp
    src/objectinterface.h
    src/objectproperties.cpp
    src/objectproperties.h
    src/plugin.cpp
    src/plugin.h
    src/plugininterface.cpp
    src/plugininterface.h
    src/queue.h
    src/stream.cpp
    src/stream.h)

if (APPLE OR FAKE_APPLE)
    add_library(VirtualCamera_cmio SHARED ${SOURCES})
else ()
    add_library(VirtualCamera_cmio SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

if (APPLE OR FAKE_APPLE)
    configure_file(Info.plist.in ${CMAKE_BINARY_DIR}/${BUILDDIR}/${EXECPREFIX}/Info.plist)
endif ()

set_target_properties(VirtualCamera_cmio PROPERTIES
                      OUTPUT_NAME ${AKVCAM_PLUGIN_NAME}
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR}
                      PREFIX "")

if (APPLE)
    set_target_properties(VirtualCamera_cmio PROPERTIES
                          SUFFIX "")
endif ()

add_dependencies(VirtualCamera_cmio VCamIPC_cmio PlatformUtils_cmio VCamUtils)
target_compile_definitions(VirtualCamera_cmio PRIVATE VIRTUALCAMERA_LIBRARY)
target_include_directories(VirtualCamera_cmio
                           PRIVATE ..
                           PRIVATE ../..)

if (FAKE_APPLE)
    target_include_directories(VirtualCamera_cmio
                               PRIVATE
                               ../FakeAPI)
endif ()

set(CMIO_PLUGIN_VENDOR "Webcamoid Project")
set(CMIO_PLUGIN_PRODUCT ${AKVCAM_PLUGIN_NAME})
add_definitions(-DCMIO_PLUGIN_VENDOR="${CMIO_PLUGIN_VENDOR}"
                -DCMIO_PLUGIN_PRODUCT="${CMIO_PLUGIN_PRODUCT}")

if (APPLE OR FAKE_APPLE)
    if (APPLE)
        find_library(COREFOUNDATION_FRAMEWORK NAMES CoreFoundation)
        find_library(COREGRAPHICS_FRAMEWORK NAMES CoreGraphics)
        find_library(COREMEDIA_FRAMEWORK NAMES CoreMedia)
        find_library(COREMEDIAIO_FRAMEWORK NAMES CoreMediaIO)
        find_library(COREVIDEO_FRAMEWORK NAMES CoreVideo)
        find_library(FOUNDATION_FRAMEWORK NAMES Foundation)
        find_library(IOKIT_FRAMEWORK NAMES IOKit)
        set(EXTRA_LIBS
            ${COREFOUNDATION_FRAMEWORK}
            ${COREGRAPHICS_FRAMEWORK}
            ${COREMEDIA_FRAMEWORK}
            ${COREMEDIAIO_FRAMEWORK}
            ${COREVIDEO_FRAMEWORK}
            ${FOUNDATION_FRAMEWORK}
            ${IOKIT_FRAMEWORK})
    endif ()
endif ()

target_link_libraries(VirtualCamera_cmio
                      VCamIPC_cmio
                      PlatformUtils_cmio
                      VCamUtils
                      ${EXTRA_LIBS})

if (FAKE_APPLE)
    add_definitions(-DFAKE_APPLE)
endif ()

if (APPLE OR FAKE_APPLE)
    install(TARGETS VirtualCamera_cmio DESTINATION ${BINDIR})
    install(FILES ${CMAKE_BINARY_DIR}/${BUILDDIR}/${EXECPREFIX}/Info.plist DESTINATION ${EXECPREFIX})
endif ()
