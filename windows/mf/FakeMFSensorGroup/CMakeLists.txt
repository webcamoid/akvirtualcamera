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

project(FakeMFSensorGroup LANGUAGES CXX)

include(../../../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# this option allow testing MFCreateVirtualCamera in Wine, Windows 10 and older.
# NOTE: No, this option won't provide the real functionality in older systems,
# this is just for testing purposes without having Windows 11+ or with an
# incomplete API.
set(FAKE_MFSENSORGROUP OFF CACHE BOOL "Create a fake mfsensorgroup.dll for testing in Wine, Windows 10 and older.")

set(SOURCES
    src/mfvcamimpl.h
    src/mfvcamimpl.cpp
    src/plugin.cpp
    FakeMFSensorGroup.def)

if (WIN32 AND FAKE_MFSENSORGROUP)
    add_library(FakeMFSensorGroup SHARED ${SOURCES})
else ()
    add_library(FakeMFSensorGroup SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(FakeMFSensorGroup PROPERTIES
                      OUTPUT_NAME mfsensorgroup
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR}
                      PREFIX "")
add_dependencies(FakeMFSensorGroup PlatformUtils_windows VCamUtils)
target_compile_definitions(FakeMFSensorGroup PRIVATE FAKEMFSENSORGROUP_LIBRARY)
target_include_directories(FakeMFSensorGroup
                           PRIVATE
                           ../MediaSource/src
                           ..
                           ../..
                           ../../..)

if (WIN32 AND FAKE_MFSENSORGROUP)
    set(EXTRA_LIBS
        ole32)
endif ()

target_link_libraries(FakeMFSensorGroup
                      PlatformUtils_windows
                      VCamUtils
                      ${EXTRA_LIBS})
add_definitions(-DSTRSAFE_NO_DEPRECATE)

if (WIN32 AND FAKE_MFSENSORGROUP)
    install(TARGETS FakeMFSensorGroup DESTINATION ${BINDIR})
endif ()
