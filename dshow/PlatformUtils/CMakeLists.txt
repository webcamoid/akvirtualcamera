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

project(PlatformUtils LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(../dshow.cmake)

add_library(PlatformUtils STATIC
            src/messagecommons.h
            src/messageserver.cpp
            src/messageserver.h
            src/mutex.cpp
            src/mutex.h
            src/preferences.cpp
            src/preferences.h
            src/sharedmemory.cpp
            src/sharedmemory.h
            src/utils.cpp
            src/utils.h)

add_dependencies(PlatformUtils VCamUtils)
target_compile_definitions(PlatformUtils PRIVATE PLATFORMUTILS_LIBRARY)
target_include_directories(PlatformUtils PRIVATE ../..)
target_link_libraries(PlatformUtils 
                      VCamUtils
                      advapi32
                      kernel32
                      gdi32
                      shell32
                      windowscodecs)
