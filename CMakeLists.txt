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

project(AkVirtualCamera)

include(commons.cmake)

add_subdirectory(VCamUtils)

if (APPLE)
    add_subdirectory(cmio)
elseif (WIN32)
    add_subdirectory(dshow)
endif ()

add_subdirectory(Manager)

if (APPLE)
    set(TARGET_SYSTEM mac)
elseif (WIN32)
    set(TARGET_SYSTEM windows)
endif ()

file(WRITE ${CMAKE_BINARY_DIR}/package_info.conf
     "[Package]\n"
     "appName = AkVirtualCamera\n"
     "packageName = akvirtualcamera\n"
     "version = ${VERSION}\n"
     "description = AkVirtualCamera, virtual camera for Mac and Windows\n"
     "url = https://github.com/webcamoid/akvirtualcamera\n"
     "licenseDescription = GNU General Public License v3.0\n"
     "targetSystem = ${TARGET_SYSTEM}\n")
