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

# Allow to build the virtual camera in Linux and other POSIX systems as if you
# were building it for APPLE
# NOTE: No, this option isn't for cross compile, the resulting binaries are not
# compatible with Mac, this option allows to test the virtual camera outside
# Mac.
set(FAKE_APPLE OFF CACHE BOOL "Virtual camera in Linux and other POSIX systems as if you were building it for APPLE")

if (NOT (APPLE OR FAKE_APPLE) AND NOT WIN32)
    message(FATAL_ERROR "This driver only works in Mac an Windows. For Linux check 'akvcam' instead.")
endif ()

include(CheckCXXSourceCompiles)

set(COMMONS_APPNAME AkVirtualCamera)
string(TOLOWER ${COMMONS_APPNAME} COMMONS_TARGET)

set(VER_MAJ 9)
set(VER_MIN 1)
set(VER_PAT 3)
set(VERSION ${VER_MAJ}.${VER_MIN}.${VER_PAT})
set(DAILY_BUILD OFF CACHE BOOL "Mark this as a daily build")

add_definitions(-DCOMMONS_APPNAME="${COMMONS_APPNAME}"
                -DCOMMONS_TARGET="${COMMONS_TARGET}"
                -DCOMMONS_VER_MAJ="${VER_MAJ}"
                -DCOMMONS_VERSION="${VERSION}"
                -DPREFIX="${PREFIX}")

set(AKVCAM_PLUGIN_NAME AkVirtualCamera)
set(AKVCAM_SERVICE_NAME AkVCamAssistant)
set(AKVCAM_MANAGER_NAME AkVCamManager)
set(AKVCAM_DEVICE_PREFIX AkVCamVideoDevice)
set(AKVCAM_SERVICEPORT "8226" CACHE STRING "Virtual camera service port")

add_definitions(-DAKVCAM_PLUGIN_NAME="${AKVCAM_PLUGIN_NAME}"
                -DAKVCAM_SERVICE_NAME="${AKVCAM_SERVICE_NAME}"
                -DAKVCAM_MANAGER_NAME="${AKVCAM_MANAGER_NAME}"
                -DAKVCAM_DEVICE_PREFIX="${AKVCAM_DEVICE_PREFIX}")

if (WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT FAKE_APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (DAILY_BUILD)
    add_definitions(-DDAILY_BUILD)
endif ()

set(GIT_COMMIT_HASH "" CACHE STRING "Set the alternative commit hash if not detected by git")

find_program(GIT_BIN git)

if (GIT_BIN)
    execute_process(COMMAND ${GIT_BIN} rev-parse HEAD
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    OUTPUT_VARIABLE GIT_COMMIT_HASH_RESULT
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)

endif ()

if ("${GIT_COMMIT_HASH_RESULT}" STREQUAL "")
    set(GIT_COMMIT_HASH_RESULT "${GIT_COMMIT_HASH}")
endif ()

if (NOT "${GIT_COMMIT_HASH_RESULT}" STREQUAL "")
    add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH_RESULT}")
endif ()

# NOTE for other developers: TARGET_ARCH is intended to be used as a reference
# for the deploy tool, so don't rush on adding new architectures unless you
# want to create a binary distributable for that architecture.
# AkVirtualCamera build is not affected in anyway by the value of TARGET_ARCH,
# if the build fails its something else and totally unrelated to that variable.

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
        set(QTIFW_TARGET_DIR "\@ApplicationsDirX64\@/${AKVCAM_PLUGIN_NAME}")
    else ()
        set(QTIFW_TARGET_DIR "\@ApplicationsDirX86\@/${AKVCAM_PLUGIN_NAME}")
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

if (APPLE OR FAKE_APPLE)
    set(BUILDDIR build)
    set(EXECPREFIX ${AKVCAM_PLUGIN_NAME}.plugin/Contents)
    set(BINDIR ${EXECPREFIX}/MacOS)
    set(LIBDIR ${EXECPREFIX}/Frameworks)
    set(DATAROOTDIR ${EXECPREFIX}/Resources)
    set(LICENSEDIR ${DATAROOTDIR})
elseif (WIN32)
    include(GNUInstallDirs)
    set(BUILDDIR build)
    set(EXECPREFIX "")
    set(BINDIR ${WIN_TARGET_ARCH})
    set(LIBDIR ${CMAKE_INSTALL_LIBDIR}/${WIN_TARGET_ARCH})
    set(DATAROOTDIR ${CMAKE_INSTALL_DATAROOTDIR})
    set(LICENSEDIR ${DATAROOTDIR}/licenses/${AKVCAM_PLUGIN_NAME})
endif ()

if (APPLE OR FAKE_APPLE)
    set(TARGET_PLATFORM mac)
    set(BUILD_INFO_FILE ${DATAROOTDIR}/build-info.txt)
    set(APP_LIBDIR ${LIBDIR})
    set(MAIN_EXECUTABLE ${BINDIR}/${AKVCAM_PLUGIN_NAME})
    set(PACKET_HIDE_ARCH false)
    set(QTIFW_TARGET_DIR "\@ApplicationsDir\@/${AKVCAM_PLUGIN_NAME}")
    set(OUTPUT_FORMATS "MacPkg")
elseif (WIN32)
    set(TARGET_PLATFORM windows)
    set(BUILD_INFO_FILE ${DATAROOTDIR}/build-info.txt)
    set(MAIN_EXECUTABLE ${BINDIR}/${AKVCAM_MANAGER_NAME}.exe)
    set(APP_LIBDIR ${WIN_TARGET_ARCH})
    set(PACKET_HIDE_ARCH true)
    set(OUTPUT_FORMATS "Nsis")
endif ()
