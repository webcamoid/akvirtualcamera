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
    set(TARGET_PLATFORM mac)
    set(BUILD_INFO_FILE AkVirtualCamera.plugin/Contents/Resources/build-info.txt)
    set(APP_LIBDIR AkVirtualCamera.plugin/Contents/Frameworks)
    set(MAIN_EXECUTABLE AkVirtualCamera.plugin/Contents/MacOS/AkVirtualCamera)
    set(PACKET_HIDE_ARCH false)

    if (IS_ARM_TARGET)
        if (IS_64BITS_TARGET)
            set(PACKET_TARGET_ARCH arm64)
        else ()
            set(PACKET_TARGET_ARCH arm32)
        endif()
    else ()
        if (IS_64BITS_TARGET)
            set(PACKET_TARGET_ARCH x64)
        else ()
            set(PACKET_TARGET_ARCH x86)
        endif()
    endif()

    set(QTIFW_TARGET_DIR "\@ApplicationsDir\@/AkVirtualCamera")
    set(OUTPUT_FORMATS "MacPkg")
elseif (WIN32)
    add_subdirectory(dshow)
    set(TARGET_PLATFORM windows)
    set(BUILD_INFO_FILE share/build-info.txt)
    set(MAIN_EXECUTABLE ${TARGET_ARCH}/AkVCamManager.exe)
    set(APP_LIBDIR ${TARGET_ARCH})
    set(PACKET_HIDE_ARCH true)

    if (IS_64BITS_TARGET)
        set(PACKET_TARGET_ARCH win64)
    else ()
        set(PACKET_TARGET_ARCH win32)
    endif()

    set(OUTPUT_FORMATS "Nsis")
endif ()

add_subdirectory(Manager)

configure_file(package_info.conf.in package_info.conf)
