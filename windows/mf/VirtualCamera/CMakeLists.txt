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

project(VirtualCamera_mf LANGUAGES CXX)

include(../../../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/classfactory.h
    src/classfactory.cpp
    src/plugin.cpp
    src/plugin.h
    src/plugininterface.cpp
    src/plugininterface.h
    VirtualCamera.def)

if (WIN32)
    add_library(VirtualCamera_mf SHARED ${SOURCES})
else ()
    add_library(VirtualCamera_mf SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VirtualCamera_mf PROPERTIES
                      OUTPUT_NAME ${AKVCAM_PLUGIN_MF_NAME}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR}
                      PREFIX "")
add_dependencies(VirtualCamera_mf VCamMediaSource PlatformUtils_windows VCamUtils MFUtils)
target_compile_definitions(VirtualCamera_mf PRIVATE VIRTUALCAMERAMF_LIBRARY)
target_include_directories(VirtualCamera_mf
                           PRIVATE
                           ../MediaSource/src
                           ..
                           ../..
                           ../../..)

if (WIN32)
    set(EXTRA_LIBS
        mfplat
        mfuuid
        winmm)
endif ()

target_link_libraries(VirtualCamera_mf
                      VCamMediaSource
                      VCamIPC_windows
                      PlatformUtils_windows
                      VCamUtils
                      MFUtils
                      ${EXTRA_LIBS})
add_definitions(-DNOMINMAX
                -DSTRSAFE_NO_DEPRECATE)

if (WIN32)
    install(TARGETS VirtualCamera_mf DESTINATION ${BINDIR})
endif ()
