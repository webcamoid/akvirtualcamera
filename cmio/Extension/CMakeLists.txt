# akvirtualcamera, virtual camera for Mac and Windows.
# Copyright (C) 2026  Gonzalo Exequiel Pedone
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

project(CameraExtension LANGUAGES CXX OBJCXX)

include(../../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(COREMEDIAIO NAMES CoreMediaIO)
find_library(COREMEDIA NAMES CoreMedia)
find_library(COREVIDEO NAMES CoreVideo)
find_library(FOUNDATION NAMES Foundation)
find_library(COREAUDIO NAMES CoreAudio)

set(SOURCES src/main.mm)
set(SOURCES_EXTRAS
    src/extensionprovidersource.h
    src/extensionprovidersource.mm
    src/extensiondevicesource.h
    src/extensiondevicesource.mm
    src/extensionstreamsource.h
    src/extensionstreamsource.mm)

if (NOT FAKE_APPLE)
    list(APPEND SOURCES ${SOURCES_EXTRAS})
 endif ()

if (FAKE_APPLE
    OR (COREMEDIAIO
        AND COREMEDIA
        AND COREVIDEO
        AND FOUNDATION
        AND COREAUDIO))
    add_executable(CameraExtension ${SOURCES})

    if (FAKE_APPLE)
        add_executable(CameraExtensionExtras EXCLUDE_FROM_ALL ${SOURCES_EXTRAS})
    endif ()
else ()
    add_executable(CameraExtension EXCLUDE_FROM_ALL ${SOURCES})
endif ()

if (APPLE)
    target_compile_options(CameraExtension PRIVATE -fobjc-arc)
endif ()

set(BUNDLE_NAME ${APP_IDENTIFIER}CX.Extension)
set(BUNDLE_EXECPREFIX ${AKVCAM_PLUGIN_NAME}CX.app/Contents/Library/SystemExtensions/${BUNDLE_NAME}.systemextension/Contents)
set(BUNDLE_ID ${BUNDLE_NAME})
add_definitions(-DBUNDLE_ID="${BUNDLE_ID}")

set_target_properties(CameraExtension PROPERTIES
                      OUTPUT_NAME ${BUNDLE_NAME}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BUNDLE_EXECPREFIX}/MacOS
                      PREFIX "")

add_dependencies(CameraExtension VCamIPC_cmio PlatformUtils_cmio VCamUtils)
target_compile_definitions(CameraExtension PRIVATE VIRTUALCAMERA_EXTENSION_APP)
target_include_directories(CameraExtension
                           PRIVATE ..
                           PRIVATE ../..)

if (FAKE_APPLE)
    target_include_directories(CameraExtension
                               PRIVATE
                               ../FakeAPI)
endif ()

if (APPLE OR FAKE_APPLE)
    configure_file(Info.plist.in ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BUNDLE_EXECPREFIX}/Info.plist)
endif ()

target_link_libraries(CameraExtension
                      VCamIPC_cmio
                      PlatformUtils_cmio
                      VCamUtils)

if (COREMEDIAIO
    AND COREMEDIA
    AND COREVIDEO
    AND FOUNDATION
    AND COREAUDIO)
    target_link_libraries(CameraExtension
                          ${COREMEDIAIO}
                          ${COREMEDIA}
                          ${COREVIDEO}
                          ${FOUNDATION}
                          ${COREAUDIO})
endif ()

if (APPLE OR FAKE_APPLE)
    install(TARGETS CameraExtension DESTINATION ${BUNDLE_EXECPREFIX}/MacOS)
    install(FILES ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BUNDLE_EXECPREFIX}/Info.plist DESTINATION ${BUNDLE_EXECPREFIX})
endif ()
