# akvirtualcamera, virtual camera for Mac and Windows.
# Copyright (C) 2026  Gonzalo Exequiel Pedone
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

project(AppDelegate LANGUAGES CXX OBJCXX)

include(../../commons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(COCOA NAMES Cocoa)
find_library(SYSTEMEXTENSIONS NAMES SystemExtensions)
find_library(FOUNDATION NAMES Foundation)

set(SOURCES src/main.mm)
set(SOURCES_EXTRAS
    src/appdelegate.h
    src/appdelegate.mm)

if (NOT FAKE_APPLE)
    list(APPEND SOURCES ${SOURCES_EXTRAS})
endif ()

if (FAKE_APPLE
    OR (COCOA
        AND SYSTEMEXTENSIONS
        AND FOUNDATION))
    add_executable(AppDelegate ${SOURCES})

    if (FAKE_APPLE)
        add_executable(AppDelegateExtras EXCLUDE_FROM_ALL ${SOURCES_EXTRAS})
    endif ()
else ()
    add_executable(AppDelegate EXCLUDE_FROM_ALL ${SOURCES})
endif ()

if (APPLE)
    target_compile_options(AppDelegate PRIVATE -fobjc-arc)
endif ()

set(BUNDLE_NAME ${COMMONS_APPNAME}CX)
set(BUNDLE_EXECPREFIX ${BUNDLE_NAME}.app/Contents)
set(BUNDLE_ID ${APP_IDENTIFIER}CX)
set(EXTENSION_BUNDLE_ID ${APP_IDENTIFIER}CX.Extension)
add_definitions(-DEXTENSION_BUNDLE_ID="${EXTENSION_BUNDLE_ID}")

set_target_properties(AppDelegate PROPERTIES
                      OUTPUT_NAME ${BUNDLE_NAME}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BUNDLE_EXECPREFIX}/MacOS
                      PREFIX "")

target_compile_definitions(AppDelegate PRIVATE VIRTUALCAMERA_DELEGATE_APP)
target_include_directories(AppDelegate
                           PRIVATE ..
                           PRIVATE ../..)

if (FAKE_APPLE)
    target_include_directories(AppDelegate
                               PRIVATE
                               ../FakeAPI)
endif ()

if (APPLE OR FAKE_APPLE)
    configure_file(Info.plist.in ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BUNDLE_EXECPREFIX}/Info.plist)
endif ()

if (COCOA
    AND SYSTEMEXTENSIONS
    AND FOUNDATION)
    target_link_libraries(AppDelegate
                          ${COCOA}
                          ${SYSTEMEXTENSIONS}
                          ${FOUNDATION})
endif ()

if (APPLE OR FAKE_APPLE)
    install(TARGETS AppDelegate DESTINATION ${BUNDLE_EXECPREFIX}/MacOS)
    install(FILES ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BUNDLE_EXECPREFIX}/Info.plist DESTINATION ${BUNDLE_EXECPREFIX})
endif ()
