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

set(COMMONS_APPNAME AkVirtualCamera)
string(TOLOWER ${COMMONS_APPNAME} COMMONS_TARGET)

set(VER_MAJ 9)
set(VER_MIN 0)
set(VER_PAT 0)
set(VERSION ${VER_MAJ}.${VER_MIN}.${VER_PAT})

add_definitions(-DCOMMONS_APPNAME="\\"${COMMONS_APPNAME}\\""
                -DCOMMONS_TARGET="\\"${COMMONS_TARGET}\\""
                -DCOMMONS_VER_MAJ="\\"${VER_MAJ}\\""
                -DCOMMONS_VERSION="\\"${VERSION}\\""
                -DPREFIX="\\"${PREFIX}\\"")

if (WIN32)
    include(CheckCXXSourceCompiles)

    check_cxx_source_compiles("
    #ifndef _WIN64
        #error Not x64
    #endif

    int main()
    {
        return 0;
    }" IS_WIN64_TARGET)

    if (IS_WIN64_TARGET)
        set(TARGET_ARCH x64 CACHE INTERNAL "")
    else ()
        set(TARGET_ARCH x86 CACHE INTERNAL "")
    endif()

    add_definitions(-DUNICODE -D_UNICODE)
endif ()

if (NOT APPLE AND NOT WIN32)
    message(FATAL_ERROR "This driver only works in Mac an Windows. For Linux check 'akvcam' instead.")
endif ()
