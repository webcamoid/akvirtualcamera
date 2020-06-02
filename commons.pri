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

COMMONS_APPNAME = "AkVirtualCamera"
COMMONS_TARGET = $$lower($${COMMONS_APPNAME})
COMMONS_TARGET = $$replace(COMMONS_TARGET, lib, "")

VER_MAJ = 9
VER_MIN = 0
VER_PAT = 0
VERSION = $${VER_MAJ}.$${VER_MIN}.$${VER_PAT}

isEmpty(BUNDLENAME): BUNDLENAME = $${COMMONS_APPNAME}

win32 {
    host_name = $$lower($$QMAKE_HOST.os)

    !isEmpty(ProgramW6432) {
        DEFAULT_PREFIX = $(ProgramW6432)/$${BUNDLENAME}
    } else: !isEmpty(ProgramFiles) {
        DEFAULT_PREFIX = $(ProgramFiles)/$${BUNDLENAME}
    } else: contains(host_name, linux) {
        DEFAULT_PREFIX = /$${BUNDLENAME}
    } else {
        DEFAULT_PREFIX = C:/$${BUNDLENAME}
    }
} else: macx: isEmpty(NOAPPBUNDLE) {
    DEFAULT_PREFIX = /Applications
} else {
    DEFAULT_PREFIX = $$[QT_INSTALL_PREFIX]
}

isEmpty(PREFIX): PREFIX = $${DEFAULT_PREFIX}
isEmpty(EXECPREFIX) {
    macx: isEmpty(NOAPPBUNDLE) {
        EXECPREFIX = $${PREFIX}/$${BUNDLENAME}.app/Contents
    } else {
        EXECPREFIX = $${PREFIX}
    }
}
isEmpty(BINDIR) {
    macx: isEmpty(NOAPPBUNDLE) {
        BINDIR = $${EXECPREFIX}/MacOS
    } else {
        BINDIR = $${EXECPREFIX}/bin
    }
}
isEmpty(SBINDIR): SBINDIR = $${EXECPREFIX}/sbin
isEmpty(LIBEXECDIR): LIBEXECDIR = $${EXECPREFIX}/libexec
isEmpty(DATAROOTDIR) {
    macx: isEmpty(NOAPPBUNDLE) {
        DATAROOTDIR = $${EXECPREFIX}/Resources
    } else {
        DATAROOTDIR = $${PREFIX}/share
    }
}
isEmpty(DATDIR): DATDIR = $${DATAROOTDIR}/$${COMMONS_TARGET}
isEmpty(SYSCONFDIR): SYSCONFDIR = $${PREFIX}/etc
isEmpty(SHAREDSTATEDIR): SHAREDSTATEDIR = $${PREFIX}/com
isEmpty(LOCALSTATEDIR): LOCALSTATEDIR = $${PREFIX}/var
isEmpty(INCLUDEDIR): INCLUDEDIR = $${PREFIX}/include
isEmpty(DOCDIR): DOCDIR = $${DATAROOTDIR}/doc/$${COMMONS_TARGET}
isEmpty(INFODIR): INFODIR = $${DATAROOTDIR}/info
isEmpty(HTMLDIR): HTMLDIR = $${DOCDIR}/html
isEmpty(DVIDIR): DVIDIR = $${DOCDIR}/dvi
isEmpty(PDFDIR): PDFDIR = $${DOCDIR}/pdf
isEmpty(PSDIR): PSDIR = $${DOCDIR}/ps
isEmpty(LIBDIR) {
    macx: isEmpty(NOAPPBUNDLE) {
        LIBDIR = $${EXECPREFIX}/Frameworks
    } else {
        INSTALL_LIBS = $$[QT_INSTALL_LIBS]
        LIBDIR = $$replace(INSTALL_LIBS, $$[QT_INSTALL_PREFIX], $${EXECPREFIX})
    }
}
isEmpty(LOCALEDIR): LOCALEDIR = $${DATAROOTDIR}/locale
isEmpty(MANDIR): MANDIR = $${DATAROOTDIR}/man
isEmpty(LICENSEDIR): LICENSEDIR = $${DATAROOTDIR}/licenses/$${COMMONS_TARGET}
isEmpty(LOCALDIR): LOCALDIR = $${PREFIX}/local
isEmpty(LOCALLIBDIR): LOCALLIBDIR = $${LOCALDIR}/lib
isEmpty(INSTALLQMLDIR) {
    macx: isEmpty(NOAPPBUNDLE) {
        INSTALLQMLDIR = $${DATAROOTDIR}/qml
    } else {
        INSTALL_QML = $$[QT_INSTALL_QML]
        INSTALLQMLDIR = $$replace(INSTALL_QML, $$[QT_INSTALL_LIBS], $${LIBDIR})
    }
}
isEmpty(INSTALLPLUGINSDIR) {
    macx: isEmpty(NOAPPBUNDLE) {
        INSTALLPLUGINSDIR = $${EXECPREFIX}/Plugins/$${COMMONS_TARGET}
    } else {
        INSTALLPLUGINSDIR = $${LIBDIR}/$${COMMONS_TARGET}
    }
}
isEmpty(JARDIR): JARDIR = $${EXECPREFIX}/libs

macx: !isEmpty(NOAPPBUNDLE): DEFINES += NOAPPBUNDLE

DEFINES += \
    COMMONS_APPNAME=\"\\\"$$COMMONS_APPNAME\\\"\" \
    COMMONS_TARGET=\"\\\"$$COMMONS_TARGET\\\"\" \
    COMMONS_VER_MAJ=\"\\\"$$VER_MAJ\\\"\" \
    COMMONS_VERSION=\"\\\"$$VERSION\\\"\" \
    PREFIX=\"\\\"$$PREFIX\\\"\" \
    EXECPREFIX=\"\\\"$$EXECPREFIX\\\"\" \
    BINDIR=\"\\\"$$BINDIR\\\"\" \
    SBINDIR=\"\\\"$$SBINDIR\\\"\" \
    LIBEXECDIR=\"\\\"$$LIBEXECDIR\\\"\" \
    DATAROOTDIR=\"\\\"$$DATAROOTDIR\\\"\" \
    DATDIR=\"\\\"$$DATDIR\\\"\" \
    SYSCONFDIR=\"\\\"$$SYSCONFDIR\\\"\" \
    SHAREDSTATEDIR=\"\\\"$$SHAREDSTATEDIR\\\"\" \
    LOCALSTATEDIR=\"\\\"$$LOCALSTATEDIR\\\"\" \
    INCLUDEDIR=\"\\\"$$INCLUDEDIR\\\"\" \
    DOCDIR=\"\\\"$$DOCDIR\\\"\" \
    INFODIR=\"\\\"$$INFODIR\\\"\" \
    HTMLDIR=\"\\\"$$HTMLDIR\\\"\" \
    DVIDIR=\"\\\"$$DVIDIR\\\"\" \
    PDFDIR=\"\\\"$$PDFDIR\\\"\" \
    PSDIR=\"\\\"$$PSDIR\\\"\" \
    LIBDIR=\"\\\"$$LIBDIR\\\"\" \
    LOCALEDIR=\"\\\"$$LOCALEDIR\\\"\" \
    MANDIR=\"\\\"$$MANDIR\\\"\" \
    LICENSEDIR=\"\\\"$$LICENSEDIR\\\"\" \
    LOCALDIR=\"\\\"$$LOCALDIR\\\"\" \
    LOCALLIBDIR=\"\\\"$$LOCALLIBDIR\\\"\" \
    INSTALLQMLDIR=\"\\\"$$INSTALLQMLDIR\\\"\" \
    INSTALLPLUGINSDIR=\"\\\"$$INSTALLPLUGINSDIR\\\"\"

msvc {
    TARGET_ARCH = $${QMAKE_TARGET.arch}
    TARGET_ARCH = $$basename(TARGET_ARCH)
    TARGET_ARCH = $$replace(TARGET_ARCH, x64, x86_64)
} else {
    TARGET_ARCH = $$system($${QMAKE_CXX} -dumpmachine)
    TARGET_ARCH = $$split(TARGET_ARCH, -)
    TARGET_ARCH = $$first(TARGET_ARCH)
}

COMPILER = $$basename(QMAKE_CXX)
COMPILER = $$replace(COMPILER, \+\+, pp)
COMPILER = $$join(COMPILER, _)

CONFIG(debug, debug|release) {
    COMMONS_BUILD_PATH = debug/$${COMPILER}/$${TARGET_ARCH}
    DEFINES += QT_DEBUG
} else {
    COMMONS_BUILD_PATH = release/$$COMPILER/$${TARGET_ARCH}
}

android: COMMONS_BUILD_PATH = $${COMMONS_BUILD_PATH}/$${ANDROID_PLATFORM}

BIN_DIR = $${COMMONS_BUILD_PATH}/bin
MOC_DIR = $${COMMONS_BUILD_PATH}/moc
OBJECTS_DIR = $${COMMONS_BUILD_PATH}/obj
RCC_DIR = $${COMMONS_BUILD_PATH}/rcc
UI_DIR = $${COMMONS_BUILD_PATH}/ui

win32 {
    CONFIG += skip_target_version_ext
    !isEmpty(STATIC_BUILD):!isEqual(STATIC_BUILD, 0) {
        win32-g++: QMAKE_LFLAGS = -static-libgcc -static-libstdc++
    }
}

# Enable c++11 support in all platforms
!CONFIG(c++11): CONFIG += c++11

!win32: !macx {
    error("This driver only works in Mac an Windows. For Linux check 'akvcam' instead.")
}

CMD_SEP = $$escape_expand(\n\t)
