# akvirtualcamera, virtual camera for Mac and Windows.
# Copyright (C) 2020  Gonzalo Exequiel Pedone
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

exists(commons.pri) {
    include(commons.pri)
} else {
    exists(../../commons.pri) {
        include(../../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

include(../dshow.pri)

CONFIG += \
    staticlib \
    create_prl \
    no_install_prl
CONFIG -= qt

DESTDIR = $${OUT_PWD}/$${BIN_DIR}

TARGET = PlatformUtils

TEMPLATE = lib

LIBS = \
    -L$${OUT_PWD}/../../VCamUtils/$${BIN_DIR} -lVCamUtils \
    -ladvapi32 \
    -lkernel32 \
    -lgdi32 \
    -lshell32 \
    -lwindowscodecs

SOURCES = \
    src/messageserver.cpp \
    src/mutex.cpp \
    src/preferences.cpp \
    src/utils.cpp \
    src/sharedmemory.cpp

HEADERS =  \
    src/messagecommons.h \
    src/messageserver.h \
    src/mutex.h \
    src/preferences.h \
    src/utils.h \
    src/sharedmemory.h

INCLUDEPATH += ../..
