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

if (NOT APPLE AND NOT WIN32)
    message(FATAL_ERROR "This driver only works in Mac an Windows. For Linux check 'akvcam' instead.")
endif ()

include(CheckCXXSourceCompiles)

set(COMMONS_APPNAME AkVirtualCamera)
string(TOLOWER ${COMMONS_APPNAME} COMMONS_TARGET)

set(VER_MAJ 9)
set(VER_MIN 1)
set(VER_PAT 2)
set(VERSION ${VER_MAJ}.${VER_MIN}.${VER_PAT})
set(DAILY_BUILD OFF CACHE BOOL "Mark this as a daily build")

add_definitions(-DCOMMONS_APPNAME="${COMMONS_APPNAME}"
                -DCOMMONS_TARGET="${COMMONS_TARGET}"
                -DCOMMONS_VER_MAJ="${VER_MAJ}"
                -DCOMMONS_VERSION="${VERSION}"
                -DPREFIX="${PREFIX}")

if (WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

if (DAILY_BUILD)
    add_definitions(-DDAILY_BUILD)
endif ()

# NOTE for other developers: TARGET_ARCH is intended to be used as a reference
# for the deploy tool, so don't rush on adding new architectures unless you
# want to create a binary distributable for that architecture.
# Webcamoid build is not affected in anyway by the value of TARGET_ARCH, if the
# build fails its something else and totally unrelated to that variable.

if (WIN32)
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("
    #include <windows.h>

    #ifndef _M_X64
        #error Not WIN64
    #endif

    int main()
    {
        return 0;
    }" IS_WIN64_TARGET)

    check_cxx_source_compiles("
    #include <windows.h>

    #ifndef _M_IX86
        #error Not WIN32
    #endif

    int main()
    {
        return 0;
    }" IS_WIN32_TARGET)

    check_cxx_source_compiles("
    #include <windows.h>

    #ifndef _M_ARM64
        #error Not ARM64
    #endif

    int main()
    {
        return 0;
    }" IS_WIN64_ARM_TARGET)

    check_cxx_source_compiles("
    #include <windows.h>

    #ifndef _M_ARM
        #error Not ARM
    #endif

    int main()
    {
        return 0;
    }" IS_WIN32_ARM_TARGET)

    if (IS_WIN64_TARGET OR IS_WIN64_ARM_TARGET)
        set(QTIFW_TARGET_DIR "\@ApplicationsDirX64\@/Webcamoid")
    else ()
        set(QTIFW_TARGET_DIR "\@ApplicationsDirX86\@/Webcamoid")
    endif()

    if (IS_WIN64_TARGET)
        set(TARGET_ARCH win64)
        set(WIN_TARGET_ARCH x64)
    elseif (IS_WIN64_ARM_TARGET)
        set(TARGET_ARCH win64_arm)
        set(WIN_TARGET_ARCH arm64)
    elseif (IS_WIN32_TARGET)
        set(TARGET_ARCH win32)
        set(WIN_TARGET_ARCH x86)
    elseif (IS_WIN32_ARM_TARGET)
        set(TARGET_ARCH win32_arm)
        set(WIN_TARGET_ARCH arm32)
    else ()
        set(TARGET_ARCH unknown)
        set(WIN_TARGET_ARCH unknown)
    endif()
elseif (UNIX)
    if (ANDROID)
        set(TARGET_ARCH ${CMAKE_ANDROID_ARCH_ABI})
    else ()
        include(CheckCXXSourceCompiles)
        check_cxx_source_compiles("
        #ifndef __x86_64__
            #error Not x64
        #endif

        int main()
        {
            return 0;
        }" IS_X86_64_TARGET)

        check_cxx_source_compiles("
        #ifndef __i386__
            #error Not x86
        #endif

        int main()
        {
            return 0;
        }" IS_I386_TARGET)

        check_cxx_source_compiles("
        #ifndef __aarch64__
            #error Not ARM64
        #endif

        int main()
        {
            return 0;
        }" IS_ARM64_TARGET)

        check_cxx_source_compiles("
        #ifndef __arm__
            #error Not ARM
        #endif

        int main()
        {
            return 0;
        }" IS_ARM_TARGET)

        check_cxx_source_compiles("
        #ifndef __riscv
            #error Not RISC-V
        #endif

        int main()
        {
            return 0;
        }" IS_RISCV_TARGET)

        if (IS_X86_64_TARGET)
            set(TARGET_ARCH x64)
        elseif (IS_I386_TARGET)
            set(TARGET_ARCH x86)
        elseif (IS_ARM64_TARGET)
            set(TARGET_ARCH arm64)
        elseif (IS_ARM_TARGET)
            set(TARGET_ARCH arm32)
        elseif (IS_RISCV_TARGET)
            set(TARGET_ARCH riscv)
        else ()
            set(TARGET_ARCH unknown)
        endif ()
    endif ()
endif ()
