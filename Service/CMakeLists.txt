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

project(Service LANGUAGES CXX)

include(../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Service
               src/main.cpp
               src/service.cpp
               src/service.h)

if (APPLE OR FAKE_APPLE)
    set_target_properties(Service PROPERTIES
                          OUTPUT_NAME ${AKVCAM_SERVICE_NAME}
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${DATAROOTDIR})
else ()
    set_target_properties(Service PROPERTIES
                          OUTPUT_NAME ${AKVCAM_SERVICE_NAME}
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR}
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR})
endif ()

add_dependencies(Service
                 VCamUtils)

if (APPLE OR FAKE_APPLE)
    add_dependencies(Service
                     VCamIPC_cmio PlatformUtils_cmio)
elseif (WIN32)
    add_dependencies(Service
                     VCamIPC_windows PlatformUtils_windows)
endif ()

target_include_directories(Service
                           PRIVATE
                           ..)
target_link_libraries(Service
                      VCamUtils)

if (APPLE OR FAKE_APPLE)
    target_include_directories(Service
                               PRIVATE
                               ../cmio)

    if (FAKE_APPLE)
        target_include_directories(Service
                                   PRIVATE
                                   ../cmio/FakeAPI)
    endif ()

    target_link_libraries(Service
                          VCamIPC_cmio
                          PlatformUtils_cmio)
elseif (WIN32)
    target_include_directories(Service
                               PRIVATE
                               ../windows)
    target_link_libraries(Service
                          VCamIPC_windows
                          PlatformUtils_windows)
    add_definitions(-DNOMINMAX)
endif ()

if (WIN32)
    target_link_libraries(Service
                          ws2_32)
endif ()

if (APPLE OR FAKE_APPLE)
    install(TARGETS Service DESTINATION ${DATAROOTDIR})
elseif (WIN32)
    install(TARGETS Service DESTINATION ${BINDIR})
endif ()
