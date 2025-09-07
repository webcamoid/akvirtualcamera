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

project(VCamTest_mf LANGUAGES CXX)

include(../../../commons.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    add_executable(VCamTest_mf WIN32 src/main.cpp)
else ()
    add_executable(VCamTest_mf EXCLUDE_FROM_ALL src/main.cpp)
endif ()

set_target_properties(VCamTest_mf PROPERTIES
                      OUTPUT_NAME AkVCamTestMF
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR})
add_dependencies(VCamTest_mf VCamMediaSource MFUtils)
target_include_directories(VCamTest_mf
                           PRIVATE
                           ../MediaSource/src
                           ..
                           ../..
                           ../../..)
add_definitions(-DNOMINMAX)

if (WIN32)
    set(EXTRA_LIBS
        mfplat
        mfreadwrite
        mfuuid
        winmm)
endif ()

target_link_libraries(VCamTest_mf
                      VCamMediaSource
                      MFUtils
                      ${EXTRA_LIBS})

if (WIN32)
    install(TARGETS VCamTest_mf DESTINATION ${BINDIR})
endif ()
