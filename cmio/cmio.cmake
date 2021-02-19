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

set(CMIO_PLUGIN_NAME AkVirtualCamera)
set(CMIO_PLUGIN_ASSISTANT_NAME AkVCamAssistant)
set(CMIO_PLUGIN_DEVICE_PREFIX AkVCamVideoDevice)
set(CMIO_PLUGIN_VENDOR "Webcamoid Project")
set(CMIO_PLUGIN_PRODUCT ${CMIO_PLUGIN_NAME})
set(CMIO_ASSISTANT_NAME "org.webcamoid.cmio.AkVCam.Assistant")

add_definitions(-DCMIO_PLUGIN_NAME="\\"${CMIO_PLUGIN_NAME}\\""
                -DCMIO_PLUGIN_NAME_L="\\"${CMIO_PLUGIN_NAME}\\""
                -DCMIO_PLUGIN_ASSISTANT_NAME="\\"${CMIO_PLUGIN_ASSISTANT_NAME}\\""
                -DCMIO_PLUGIN_DEVICE_PREFIX="\\"${CMIO_PLUGIN_DEVICE_PREFIX}\\""
                -DCMIO_PLUGIN_VENDOR="\\"${CMIO_PLUGIN_VENDOR}\\""
                -DCMIO_PLUGIN_PRODUCT="\\"${CMIO_PLUGIN_PRODUCT}\\""
                -DCMIO_ASSISTANT_NAME="\\"${CMIO_ASSISTANT_NAME}\\"")
