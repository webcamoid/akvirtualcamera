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

isEmpty(CMIO_PLUGINS_DAL_PATH):
    CMIO_PLUGINS_DAL_PATH = /Library/CoreMediaIO/Plug-Ins/DAL
isEmpty(CMIO_DAEMONS_PATH):
    CMIO_DAEMONS_PATH = ~/Library/LaunchAgents
isEmpty(CMIO_PLUGIN_NAME):
    CMIO_PLUGIN_NAME = AkVirtualCamera
isEmpty(CMIO_PLUGIN_ASSISTANT_NAME):
    CMIO_PLUGIN_ASSISTANT_NAME = AkVCamAssistant
isEmpty(CMIO_PLUGIN_DEVICE_PREFIX):
    CMIO_PLUGIN_DEVICE_PREFIX = /akvcam/video
isEmpty(CMIO_PLUGIN_VENDOR):
    CMIO_PLUGIN_VENDOR = "Webcamoid Project"
isEmpty(CMIO_PLUGIN_PRODUCT):
    CMIO_PLUGIN_PRODUCT = $$CMIO_PLUGIN_NAME

DEFINES += \
    CMIO_PLUGINS_DAL_PATH=\"\\\"$$CMIO_PLUGINS_DAL_PATH\\\"\" \
    CMIO_PLUGINS_DAL_PATH_L=\"L\\\"$$CMIO_PLUGINS_DAL_PATH\\\"\" \
    CMIO_DAEMONS_PATH=\"\\\"$$CMIO_DAEMONS_PATH\\\"\" \
    CMIO_PLUGIN_NAME=\"\\\"$$CMIO_PLUGIN_NAME\\\"\" \
    CMIO_PLUGIN_NAME_L=\"L\\\"$$CMIO_PLUGIN_NAME\\\"\" \
    CMIO_PLUGIN_ASSISTANT_NAME=\"\\\"$$CMIO_PLUGIN_ASSISTANT_NAME\\\"\" \
    CMIO_PLUGIN_ASSISTANT_NAME_L=\"L\\\"$$CMIO_PLUGIN_ASSISTANT_NAME\\\"\" \
    CMIO_PLUGIN_DEVICE_PREFIX=\"\\\"$$CMIO_PLUGIN_DEVICE_PREFIX\\\"\" \
    CMIO_PLUGIN_VENDOR=\"\\\"$$CMIO_PLUGIN_VENDOR\\\"\" \
    CMIO_PLUGIN_PRODUCT=\"\\\"$$CMIO_PLUGIN_PRODUCT\\\"\"
