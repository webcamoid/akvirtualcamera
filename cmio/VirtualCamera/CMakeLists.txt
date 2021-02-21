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

project(VirtualCamera LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(../cmio.cmake)

set(CONTENTSPATH ${CMIO_PLUGIN_NAME}.plugin/Contents)
set(MACBINPATH ${CONTENTSPATH}/MacOS)

add_library(VirtualCamera SHARED
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
configure_file(Info.plist.in Info.plist)
set_target_properties(VirtualCamera PROPERTIES
                      OUTPUT_NAME ${CMIO_PLUGIN_NAME}
                      PREFIX ""
                      SUFFIX "")
add_dependencies(VirtualCamera VCamIPC PlatformUtils VCamUtils)
target_compile_definitions(VirtualCamera PRIVATE VIRTUALCAMERA_LIBRARY)
target_include_directories(VirtualCamera
                           PRIVATE ..
                           PRIVATE ../..)
find_library(COREFOUNDATION_FRAMEWORK NAMES CoreFoundation)
find_library(COREGRAPHICS_FRAMEWORK NAMES CoreGraphics)
find_library(COREMEDIA_FRAMEWORK NAMES CoreMedia)
find_library(COREMEDIAIO_FRAMEWORK NAMES CoreMediaIO)
find_library(COREVIDEO_FRAMEWORK NAMES CoreVideo)
find_library(FOUNDATION_FRAMEWORK NAMES Foundation)
find_library(IOKIT_FRAMEWORK NAMES IOKit)
find_library(IOSURFACE_FRAMEWORK NAMES IOSurface)
set(EXTRA_LIBS ${COREFOUNDATION_FRAMEWORK}
               ${COREGRAPHICS_FRAMEWORK}
               ${COREMEDIA_FRAMEWORK}
               ${COREMEDIAIO_FRAMEWORK}
               ${COREVIDEO_FRAMEWORK}
               ${FOUNDATION_FRAMEWORK}
               ${IOKIT_FRAMEWORK}
               ${IOSURFACE_FRAMEWORK})
target_link_libraries(VirtualCamera
                      PlatformUtils
                      VCamUtils
                      VCamIPC
                      ${EXTRA_LIBS})
install(TARGETS VirtualCamera DESTINATION ${MACBINPATH})
install(FILES Info.plist DESTINATION ${CONTENTSPATH})
