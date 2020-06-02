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

isEmpty(DSHOW_PLUGIN_NAME):
    DSHOW_PLUGIN_NAME = AkVirtualCamera
isEmpty(DSHOW_PLUGIN_ASSISTANT_NAME):
    DSHOW_PLUGIN_ASSISTANT_NAME = AkVCamAssistant
isEmpty(DSHOW_PLUGIN_ASSISTANT_DESCRIPTION):
    DSHOW_PLUGIN_ASSISTANT_DESCRIPTION = "Webcamoid virtual camera service"
isEmpty(DSHOW_PLUGIN_DESCRIPTION):
    DSHOW_PLUGIN_DESCRIPTION = "Webcamoid Virtual Camera"
isEmpty(DSHOW_PLUGIN_DESCRIPTION_EXT):
    DSHOW_PLUGIN_DESCRIPTION_EXT = "Central service for communicating between virtual cameras clients and servers"
isEmpty(DSHOW_PLUGIN_DEVICE_PREFIX):
    DSHOW_PLUGIN_DEVICE_PREFIX = /akvcam/video
isEmpty(DSHOW_PLUGIN_VENDOR):
    DSHOW_PLUGIN_VENDOR = "Webcamoid Project"

DEFINES += \
    DSHOW_PLUGIN_NAME=\"\\\"$$DSHOW_PLUGIN_NAME\\\"\" \
    DSHOW_PLUGIN_NAME_L=\"L\\\"$$DSHOW_PLUGIN_NAME\\\"\" \
    DSHOW_PLUGIN_ASSISTANT_NAME=\"\\\"$$DSHOW_PLUGIN_ASSISTANT_NAME\\\"\" \
    DSHOW_PLUGIN_ASSISTANT_NAME_L=\"L\\\"$$DSHOW_PLUGIN_ASSISTANT_NAME\\\"\" \
    DSHOW_PLUGIN_ASSISTANT_DESCRIPTION=\"\\\"$$DSHOW_PLUGIN_ASSISTANT_DESCRIPTION\\\"\" \
    DSHOW_PLUGIN_ASSISTANT_DESCRIPTION_L=\"L\\\"$$DSHOW_PLUGIN_ASSISTANT_DESCRIPTION\\\"\" \
    DSHOW_PLUGIN_DESCRIPTION=\"\\\"$$DSHOW_PLUGIN_DESCRIPTION\\\"\" \
    DSHOW_PLUGIN_DESCRIPTION_L=\"L\\\"$$DSHOW_PLUGIN_DESCRIPTION\\\"\" \
    DSHOW_PLUGIN_DESCRIPTION_EXT=\"\\\"$$DSHOW_PLUGIN_DESCRIPTION_EXT\\\"\" \
    DSHOW_PLUGIN_DESCRIPTION_EXT_L=\"L\\\"$$DSHOW_PLUGIN_DESCRIPTION_EXT\\\"\" \
    DSHOW_PLUGIN_DEVICE_PREFIX=\"\\\"$$DSHOW_PLUGIN_DEVICE_PREFIX\\\"\" \
    DSHOW_PLUGIN_DEVICE_PREFIX_L=\"L\\\"$$DSHOW_PLUGIN_DEVICE_PREFIX\\\"\" \
    DSHOW_PLUGIN_VENDOR=\"\\\"$$DSHOW_PLUGIN_VENDOR\\\"\" \
    DSHOW_PLUGIN_VENDOR_L=\"L\\\"$$DSHOW_PLUGIN_VENDOR\\\"\"

defineReplace(normalizedArch) {
    arch = $$replace($$1, i386, x86)
    arch = $$replace(arch, i486, x86)
    arch = $$replace(arch, i586, x86)
    arch = $$replace(arch, i686, x86)
    arch = $$replace(arch, x86_64, x64)

    return($$arch)
}
