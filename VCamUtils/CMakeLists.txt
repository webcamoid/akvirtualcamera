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

project(VCamUtils LANGUAGES CXX)

include(../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(VCamUtils STATIC
            src/algorithm.h
            src/color.h
            src/colorcomponent.cpp
            src/colorcomponent.h
            src/colorconvert.cpp
            src/colorconvert.h
            src/colorplane.cpp
            src/colorplane.h
            src/commons.h
            src/fraction.cpp
            src/fraction.h
            src/ipcbridge.h
            src/logger.cpp
            src/logger.h
            src/message.cpp
            src/message.h
            src/messageclient.cpp
            src/messageclient.h
            src/messageserver.cpp
            src/messageserver.h
            src/rect.cpp
            src/rect.h
            src/servicemsg.h
            src/settings.cpp
            src/settings.h
            src/sockets.cpp
            src/sockets.h
            src/timer.cpp
            src/timer.h
            src/utils.cpp
            src/utils.h
            src/videoadjusts.cpp
            src/videoadjusts.h
            src/videoconverter.cpp
            src/videoconverter.h
            src/videoformat.cpp
            src/videoformat.h
            src/videoformatspec.cpp
            src/videoformatspec.h
            src/videoformattypes.h
            src/videoframe.cpp
            src/videoframe.h
            src/videoframetypes.h)

if (WIN32)
    target_link_libraries(VCamUtils
                          ws2_32)
    add_definitions(-DNOMINMAX)
endif ()

target_compile_definitions(VCamUtils PRIVATE VCAMUTILS_LIBRARY)
