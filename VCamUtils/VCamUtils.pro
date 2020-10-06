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
    exists(../commons.pri) {
        include(../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

CONFIG += \
    staticlib \
    create_prl \
    no_install_prl
CONFIG -= qt

DESTDIR = $${OUT_PWD}/$${BIN_DIR}

TARGET = VCamUtils

TEMPLATE = lib

SOURCES += \
    src/fraction.cpp \
    src/image/videoformat.cpp \
    src/image/videoframe.cpp \
    src/logger.cpp \
    src/settings.cpp \
    src/timer.cpp \
    src/utils.cpp

HEADERS += \
    src/fraction.h \
    src/image/color.h \
    src/image/videoformat.h \
    src/image/videoframe.h \
    src/image/videoframetypes.h \
    src/image/videoformattypes.h \
    src/ipcbridge.h \
    src/logger.h \
    src/settings.h \
    src/timer.h \
    src/utils.h

isEmpty(STATIC_BUILD) | isEqual(STATIC_BUILD, 0) {
    win32-g++: QMAKE_LFLAGS = -static -static-libgcc -static-libstdc++
}
