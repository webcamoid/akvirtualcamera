/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
 *
 * akvirtualcamera is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * akvirtualcamera is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with akvirtualcamera. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef ASSISTANTGLOBALS_H
#define ASSISTANTGLOBALS_H

#include <functional>
#include <xpc/xpc.h>

#define AKVCAM_ASSISTANT_CLIENT_NAME "org.webcamoid.cmio.AkVCam.Client"

// General messages
#define AKVCAM_ASSISTANT_MSG_ISALIVE                 0x000
#define AKVCAM_ASSISTANT_MSG_FRAME_READY             0x001
#define AKVCAM_ASSISTANT_MSG_PICTURE_UPDATED         0x002

// Assistant messages
#define AKVCAM_ASSISTANT_MSG_REQUEST_PORT            0x100
#define AKVCAM_ASSISTANT_MSG_ADD_PORT                0x101
#define AKVCAM_ASSISTANT_MSG_REMOVE_PORT             0x102

// Device control and information
#define AKVCAM_ASSISTANT_MSG_DEVICE_UPDATE           0x200

// Device listeners controls
#define AKVCAM_ASSISTANT_MSG_DEVICE_LISTENERS        0x300
#define AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER         0x301
#define AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD     0x302
#define AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE  0x303

// Device dynamic properties
#define AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING     0x400
#define AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING  0x401
#define AKVCAM_ASSISTANT_MSG_DEVICE_CONTROLS_UPDATED 0x402

namespace AkVCam
{
    using XpcMessage = std::function<void (xpc_connection_t, xpc_object_t)>;
}

#endif // ASSISTANTGLOBALS_H
