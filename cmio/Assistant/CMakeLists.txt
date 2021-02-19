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

include(../cmio.cmake)
set(INSTALLPATH ${CMIO_PLUGIN_NAME}.plugin/Contents/Resources)

add_executable(Assistant
               src/assistant.cpp
               src/assistant.h
               src/assistantglobals.h
               src/main.cpp)
set_target_properties(Assistant PROPERTIES
                      OUTPUT_NAME ${CMIO_PLUGIN_ASSISTANT_NAME}
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALLPATH}")
add_dependencies(Assistant PlatformUtils VCamUtils)
target_include_directories(Assistant
                           PRIVATE ..
                           PRIVATE ../..)
find_library(COREFOUNDATION_FRAMEWORK NAMES CoreFoundation)
set(EXTRA_LIBS ${COREFOUNDATION_FRAMEWORK})
target_link_libraries(Assistant
                      PlatformUtils
                      VCamUtils
                      ${EXTRA_LIBS})
install(TARGETS Assistant DESTINATION ${INSTALLPATH})
