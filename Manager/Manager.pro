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

win32: include(../dshow/dshow.pri)
macx: include(../cmio/cmio.pri)
include(../VCamUtils/VCamUtils.pri)

TEMPLATE = app
CONFIG += console link_prl
CONFIG -= app_bundle
CONFIG -= qt

TARGET = manager

SOURCES = \
    src/main.cpp

INCLUDEPATH += \
    .. \
    ../..

win32: LIBS += \
    -L$${OUT_PWD}/../dshow/VCamIPC/$${BIN_DIR} -lVCamIPC \
    -L$${OUT_PWD}/../dshow/PlatformUtils/$${BIN_DIR} -lPlatformUtils \
    -ladvapi32 \
    -lgdi32 \
    -lstrmiids \
    -luuid \
    -lole32 \
    -loleaut32 \
    -lshell32
macx: LIBS += \
    -L$${OUT_PWD}/../cmio/VCamIPC/$${BIN_DIR} -lVCamIPC \
    -framework CoreFoundation \
    -framework CoreMedia \
    -framework CoreMediaIO \
    -framework CoreVideo \
    -framework Foundation \
    -framework IOKit \
    -framework IOSurface
LIBS += \
    -L$${OUT_PWD}/../VCamUtils/$${BIN_DIR} -lVCamUtils

isEmpty(STATIC_BUILD) | isEqual(STATIC_BUILD, 0) {
    win32-g++: QMAKE_LFLAGS = -static -static-libgcc -static-libstdc++
}

win32 {
    INSTALLPATH = $${DSHOW_PLUGIN_NAME}.plugin/$$normalizedArch(TARGET_ARCH)
} else {
    INSTALLPATH = $${CMIO_PLUGIN_NAME}.plugin/Contents/Resources
}

DESTDIR = $${OUT_PWD}/../$${INSTALLPATH}

INSTALLS += target
target.path = $${PREFIX}/$${INSTALLPATH}
