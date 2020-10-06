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

#ifndef MESSAGECOMMONS_H
#define MESSAGECOMMONS_H

#include <cstdint>
#include <cstring>
#include <functional>

#include "VCamUtils/src/image/videoframetypes.h"

#define AKVCAM_ASSISTANT_CLIENT_NAME "AkVCam_Client"
#define AKVCAM_ASSISTANT_SERVER_NAME "AkVCam_Server"

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

#define MSG_BUFFER_SIZE 4096
#define MAX_STRING 1024

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1)

namespace AkVCam
{
    struct Frame
    {
        uint32_t format;
        int32_t width;
        int32_t height;
        uint32_t size;
        uint8_t data[4];
    };

    struct Message
    {
        uint32_t messageId;
        uint32_t dataSize;
        uint8_t data[MSG_BUFFER_SIZE];

        Message():
            messageId(0),
            dataSize(0)
        {
            memset(this->data, 0, MSG_BUFFER_SIZE);
        }

        Message(const Message &other):
            messageId(other.messageId),
            dataSize(other.dataSize)
        {
            memcpy(this->data, other.data, MSG_BUFFER_SIZE);
        }

        Message(const Message *other):
            messageId(other->messageId),
            dataSize(other->dataSize)
        {
            memcpy(this->data, other->data, MSG_BUFFER_SIZE);
        }

        Message &operator =(const Message &other)
        {
            if (this != &other) {
                this->messageId = other.messageId;
                this->dataSize = other.dataSize;
                memcpy(this->data, other.data, MSG_BUFFER_SIZE);
            }

            return *this;
        }

        inline void clear()
        {
            this->messageId = 0;
            this->dataSize = 0;
            memset(this->data, 0, MSG_BUFFER_SIZE);
        }
    };

    template<typename T>
    inline T *messageData(Message *message)
    {
        return reinterpret_cast<T *>(message->data);
    }

    using MessageHandler = std::function<void (Message *message)>;

    struct MsgRequestPort
    {
        char port[MAX_STRING];
    };

    struct MsgAddPort
    {
        char port[MAX_STRING];
        char pipeName[MAX_STRING];
        bool status;
    };

    struct MsgRemovePort
    {
        char port[MAX_STRING];
    };

    struct MsgDeviceAdded
    {
        char device[MAX_STRING];
    };

    struct MsgDeviceRemoved
    {
        char device[MAX_STRING];
    };

    struct MsgBroadcasting
    {
        char device[MAX_STRING];
        char broadcaster[MAX_STRING];
        bool status;
    };

    struct MsgListeners
    {
        char device[MAX_STRING];
        char listener[MAX_STRING];
        size_t nlistener;
        bool status;
    };

    struct MsgIsAlive
    {
        bool alive;
    };

    struct MsgFrameReady
    {
        char device[MAX_STRING];
        char port[MAX_STRING];
    };

    struct MsgPictureUpdated
    {
        char picture[MAX_STRING];
    };

    struct MsgControlsUpdated
    {
        char device[MAX_STRING];
    };
}

#endif // MESSAGECOMMONS_H
