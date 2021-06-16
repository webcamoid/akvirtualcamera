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

cmake_minimum_required(VERSION 3.5)

project(Assistant LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(../dshow.cmake)
set(INSTALLPATH ${TARGET_ARCH})

add_executable(Assistant
               src/main.cpp
               src/service.cpp
               src/service.h)
set_target_properties(Assistant PROPERTIES
                      OUTPUT_NAME ${DSHOW_PLUGIN_ASSISTANT_NAME}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${INSTALLPATH})
add_dependencies(Assistant PlatformUtils VCamUtils)
target_include_directories(Assistant
                           PRIVATE ..
                           PRIVATE ../..)
target_link_libraries(Assistant
                      PlatformUtils
                      advapi32
                      gdi32
                      kernel32
                      ole32
                      psapi
                      shell32
                      strmiids
                      uuid)
install(TARGETS Assistant DESTINATION ${INSTALLPATH})
