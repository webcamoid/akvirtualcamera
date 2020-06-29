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

include(../cmio.pri)

CONFIG += console link_prl
CONFIG -= app_bundle
CONFIG -= qt

TARGET = $${CMIO_PLUGIN_ASSISTANT_NAME}

TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/assistant.cpp

LIBS += \
    -L$${OUT_PWD}/../PlatformUtils/$${BIN_DIR} -lPlatformUtils \
    -L$${OUT_PWD}/../../VCamUtils/$${BIN_DIR} -lVCamUtils \
    -framework CoreFoundation

HEADERS += \
    src/assistantglobals.h \
    src/assistant.h

INCLUDEPATH += \
    .. \
    ../..

INSTALLPATH = $${CMIO_PLUGIN_NAME}.plugin/Contents/Resources

DESTDIR = $${OUT_PWD}/../../$${INSTALLPATH}

INSTALLS += target
target.path = $${PREFIX}/$${INSTALLPATH}
