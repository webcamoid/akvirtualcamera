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

include("${CMAKE_CURRENT_LIST_DIR}/../commons.cmake")

set(DSHOW_PLUGIN_NAME AkVirtualCamera)
set(DSHOW_PLUGIN_ASSISTANT_NAME AkVCamAssistant)
set(DSHOW_PLUGIN_ASSISTANT_DESCRIPTION "Webcamoid virtual camera service")
set(DSHOW_PLUGIN_DESCRIPTION "Webcamoid Virtual Camera")
set(DSHOW_PLUGIN_DESCRIPTION_EXT "Central service for communicating between virtual cameras and clients")
set(DSHOW_PLUGIN_DEVICE_PREFIX AkVCamVideoDevice)
set(DSHOW_PLUGIN_VENDOR "Webcamoid Project")

add_definitions(-DDSHOW_PLUGIN_NAME="${DSHOW_PLUGIN_NAME}"
                -DDSHOW_PLUGIN_ASSISTANT_NAME="${DSHOW_PLUGIN_ASSISTANT_NAME}"
                -DDSHOW_PLUGIN_ASSISTANT_DESCRIPTION="${DSHOW_PLUGIN_ASSISTANT_DESCRIPTION}"
                -DDSHOW_PLUGIN_DESCRIPTION="${DSHOW_PLUGIN_DESCRIPTION}"
                -DDSHOW_PLUGIN_DESCRIPTION_EXT="${DSHOW_PLUGIN_DESCRIPTION_EXT}"
                -DDSHOW_PLUGIN_DEVICE_PREFIX="${DSHOW_PLUGIN_DEVICE_PREFIX}"
                -DDSHOW_PLUGIN_VENDOR="${DSHOW_PLUGIN_VENDOR}")
