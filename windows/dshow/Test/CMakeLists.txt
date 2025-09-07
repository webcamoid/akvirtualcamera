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

project(VCamTest_dshow LANGUAGES CXX)

include(../../../commons.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    add_executable(VCamTest_dshow WIN32 src/main.cpp)
else ()
    add_executable(VCamTest_dshow EXCLUDE_FROM_ALL src/main.cpp)
endif ()

set_target_properties(VCamTest_dshow PROPERTIES
                      OUTPUT_NAME AkVCamTest
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR})
add_dependencies(VCamTest_dshow VCamBaseFilter)
target_include_directories(VCamTest_dshow
                           PRIVATE
                           ../BaseFilter/src
                           ..
                           ../..
                           ../../..)

if (WIN32)
    set(EXTRA_LIBS
        winmm)
endif ()

target_link_libraries(VCamTest_dshow
                      VCamBaseFilter
                      ${EXTRA_LIBS})

add_definitions(-DNOMINMAX
                -DSTRSAFE_NO_DEPRECATE)

if (WIN32)
    install(TARGETS VCamTest_dshow DESTINATION ${BINDIR})
endif ()
