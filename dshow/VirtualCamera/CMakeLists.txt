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

include(../dshow.cmake)
set(INSTALLPATH ${TARGET_ARCH})

add_library(VirtualCamera SHARED
            src/basefilter.cpp
            src/basefilter.h
            src/classfactory.cpp
            src/classfactory.h
            src/cunknown.cpp
            src/cunknown.h
            src/enummediatypes.cpp
            src/enummediatypes.h
            src/enumpins.cpp
            src/enumpins.h
            src/filtermiscflags.cpp
            src/filtermiscflags.h
            src/latency.cpp
            src/latency.h
            src/mediafilter.cpp
            src/mediafilter.h
            src/mediasample.cpp
            src/mediasample.h
            src/mediasample2.cpp
            src/mediasample2.h
            src/memallocator.cpp
            src/memallocator.h
            src/persist.cpp
            src/persist.h
            src/persistpropertybag.cpp
            src/persistpropertybag.h
            src/pin.cpp
            src/pin.h
            src/plugin.cpp
            src/plugin.h
            src/plugininterface.cpp
            src/plugininterface.h
            src/propertyset.cpp
            src/propertyset.h
            src/pushsource.cpp
            src/pushsource.h
            src/qualitycontrol.cpp
            src/qualitycontrol.h
            src/referenceclock.cpp
            src/referenceclock.h
            src/specifypropertypages.cpp
            src/specifypropertypages.h
            src/streamconfig.cpp
            src/streamconfig.h
            src/videocontrol.cpp
            src/videocontrol.h
            src/videoprocamp.cpp
            src/videoprocamp.h
            VirtualCamera.def)
set_target_properties(VirtualCamera PROPERTIES
                      OUTPUT_NAME ${DSHOW_PLUGIN_NAME}
                      PREFIX "")
add_dependencies(VirtualCamera VCamIPC PlatformUtils VCamUtils)
target_compile_definitions(VirtualCamera PRIVATE VIRTUALCAMERA_LIBRARY)
target_include_directories(VirtualCamera
                           PRIVATE ..
                           PRIVATE ../..)
target_link_libraries(VirtualCamera
                      VCamIPC
                      PlatformUtils
                      VCamUtils
                      advapi32
                      gdi32
                      kernel32
                      ole32
                      oleaut32
                      psapi
                      shell32
                      strmiids
                      user32
                      uuid
                      winmm)
install(TARGETS VirtualCamera DESTINATION ${INSTALLPATH})
