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

project(Manager LANGUAGES CXX)

include(../commons.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Manager
               src/cmdparser.h
               src/main.cpp
               src/cmdparser.cpp)

if (APPLE OR FAKE_APPLE)
    set_target_properties(Manager PROPERTIES
                          OUTPUT_NAME ${AKVCAM_MANAGER_NAME}
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${DATAROOTDIR})
else ()
    set_target_properties(Manager PROPERTIES
                          OUTPUT_NAME ${AKVCAM_MANAGER_NAME}
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR}
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR})
endif ()

add_dependencies(Manager VCamUtils)

if (APPLE OR FAKE_APPLE)
    add_dependencies(Manager
                     VCamIPC_cmio
                     PlatformUtils_cmio)
    target_include_directories(Manager
                               PRIVATE
                               ../cmio
                               ../cmio/FakeAPI)
elseif (WIN32)
    add_dependencies(Manager
                     VCamIPC_windows
                     PlatformUtils_windows)
    target_include_directories(Manager
                               PRIVATE
                               ../windows)
     add_definitions(-DNOMINMAX)
endif ()

target_include_directories(Manager
                           PRIVATE ..)
target_link_libraries(Manager
                      VCamUtils)

if (APPLE OR FAKE_APPLE)
    target_link_libraries(Manager
                          VCamIPC_cmio
                          PlatformUtils_cmio)
elseif (WIN32)
    target_link_libraries(Manager
                          VCamIPC_windows
                          PlatformUtils_windows)
endif ()

if (APPLE OR FAKE_APPLE)
    if (APPLE)
        find_library(COREFOUNDATION_FRAMEWORK NAMES CoreFoundation)
        find_library(COREMEDIA_FRAMEWORK NAMES CoreMedia)
        find_library(COREMEDIAIO_FRAMEWORK NAMES CoreMediaIO)
        find_library(COREVIDEO_FRAMEWORK NAMES CoreVideo)
        find_library(FOUNDATION_FRAMEWORK NAMES Foundation)
        find_library(IOKIT_FRAMEWORK NAMES IOKit)

        set(EXTRA_LIBS ${COREFOUNDATION_FRAMEWORK}
                       ${COREMEDIA_FRAMEWORK}
                       ${COREMEDIAIO_FRAMEWORK}
                       ${COREVIDEO_FRAMEWORK}
                       ${FOUNDATION_FRAMEWORK}
                       ${IOKIT_FRAMEWORK})
        target_link_libraries(Manager ${EXTRA_LIBS})
    endif ()
elseif (WIN32)
    target_link_libraries(Manager
                          advapi32
                          gdi32
                          ole32
                          oleaut32
                          shell32
                          strmiids
                          uuid)
endif ()

if (APPLE OR FAKE_APPLE)
    install(TARGETS Manager DESTINATION ${DATAROOTDIR})
elseif (WIN32)
    install(TARGETS Manager DESTINATION ${BINDIR})
endif ()
