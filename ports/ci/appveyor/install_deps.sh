#!/bin/bash

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

yes | pacman --noconfirm -Syyu \
    --ignore filesystem,pacman,pacman-mirrors
yes | pacman --noconfirm --needed -S \
    cmake \
    git \
    make \
    pkg-config \
    python3 \
    mingw-w64-x86_64-binutils \
    mingw-w64-i686-binutils \
    mingw-w64-x86_64-cmake \
    mingw-w64-i686-cmake \
    mingw-w64-x86_64-pkg-config \
    mingw-w64-i686-pkg-config
