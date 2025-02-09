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
#include <functional>
#include <string>
#include <vector>

namespace AkVCam
{
    class MessagePrivate;

    class Message
    {
        public:
            Message();
            Message(int id, uint64_t queryId=0);
            Message(int id, uint64_t queryId, const std::vector<char> &data);
            Message(int id, const std::vector<char> &data);
            Message(const Message &other);
            ~Message();
            Message &operator =(const Message &other);
            bool operator ==(const Message &other) const;

            int id() const;
            uint64_t queryId() const;
            const std::vector<char> &data() const;

        private:
            MessagePrivate *d;
    };

    class MsgCommonsPrivate;

    class MsgCommons
    {
        public:
            MsgCommons(uint64_t queryId=0);
            ~MsgCommons();

            uint64_t queryId() const;

        private:
            MsgCommonsPrivate *d;

        protected:
            void setQueryId(uint64_t queryId);
    };

    class MsgStatusPrivate;

    class MsgStatus: public MsgCommons
    {
        public:
            MsgStatus(int status=0, uint64_t queryId=0);
            MsgStatus(const MsgStatus &other);
            MsgStatus(const Message &message);
            ~MsgStatus();
            MsgStatus &operator =(const MsgStatus &other);
            bool operator ==(const MsgStatus &other) const;
            operator Message() const;

            int status() const;

        private:
            MsgStatusPrivate *d;
    };

    class MsgClientsPrivate;

    class MsgClients: public MsgCommons
    {
        public:
            enum ClientType
            {
                ClientType_Any,
                ClientType_VCams,
            };

            MsgClients(ClientType clientType=ClientType_Any,
                       const std::vector<uint64_t> &clients={},
                       uint64_t queryId=0);
            MsgClients(const MsgClients &other);
            MsgClients(const Message &message);
            ~MsgClients();
            MsgClients &operator =(const MsgClients &other);
            bool operator ==(const MsgClients &other) const;
            operator Message() const;

            ClientType clientType() const;
            const std::vector<uint64_t> &clients() const;

        private:
            MsgClientsPrivate *d;
    };

    class MsgUpdateDevicesPrivate;

    class MsgUpdateDevices: public MsgCommons
    {
        public:
            MsgUpdateDevices(uint64_t queryId=0);
            MsgUpdateDevices(const MsgUpdateDevices &other);
            MsgUpdateDevices(const Message &message);
            ~MsgUpdateDevices();
            MsgUpdateDevices &operator =(const MsgUpdateDevices &other);
            bool operator ==(const MsgUpdateDevices &other) const;
            operator Message() const;

        private:
            MsgUpdateDevicesPrivate *d;
    };

    class MsgDevicesUpdatedPrivate;

    class MsgDevicesUpdated: public MsgCommons
    {
        public:
            MsgDevicesUpdated(uint64_t queryId=0);
            MsgDevicesUpdated(const MsgDevicesUpdated &other);
            MsgDevicesUpdated(const Message &message);
            ~MsgDevicesUpdated();
            MsgDevicesUpdated &operator =(const MsgDevicesUpdated &other);
            bool operator ==(const MsgDevicesUpdated &other) const;
            operator Message() const;

        private:
            MsgDevicesUpdatedPrivate *d;
    };

    class MsgUpdatePicturePrivate;

    class MsgUpdatePicture: public MsgCommons
    {
        public:
            MsgUpdatePicture(const std::string &picture={},
                              uint64_t queryId=0);
            MsgUpdatePicture(const MsgUpdatePicture &other);
            MsgUpdatePicture(const Message &message);
            ~MsgUpdatePicture();
            MsgUpdatePicture &operator =(const MsgUpdatePicture &other);
            bool operator ==(const MsgUpdatePicture &other) const;
            operator Message() const;

            const std::string &picture() const;

        private:
            MsgUpdatePicturePrivate *d;
    };

    class MsgPictureUpdatedPrivate;

    class MsgPictureUpdated: public MsgCommons
    {
        public:
            MsgPictureUpdated(const std::string &picture={},
                              bool updated=false,
                              uint64_t queryId=0);
            MsgPictureUpdated(const MsgPictureUpdated &other);
            MsgPictureUpdated(const Message &message);
            ~MsgPictureUpdated();
            MsgPictureUpdated &operator =(const MsgPictureUpdated &other);
            bool operator ==(const MsgPictureUpdated &other) const;
            operator Message() const;

            const std::string &picture() const;
            bool updated() const;

        private:
            MsgPictureUpdatedPrivate *d;
    };

    class MsgUpdateControlsPrivate;

    class MsgUpdateControls: public MsgCommons
    {
        public:
            MsgUpdateControls(const std::string &device={},
                               uint64_t queryId=0);
            MsgUpdateControls(const MsgUpdateControls &other);
            MsgUpdateControls(const Message &message);
            ~MsgUpdateControls();
            MsgUpdateControls &operator =(const MsgUpdateControls &other);
            bool operator ==(const MsgUpdateControls &other) const;
            operator Message() const;

            const std::string &device() const;

        private:
            MsgUpdateControlsPrivate *d;
    };

    class MsgControlsUpdatedPrivate;

    class MsgControlsUpdated: public MsgCommons
    {
        public:
            MsgControlsUpdated(const std::string &device={},
                               bool updated=false,
                               uint64_t queryId=0);
            MsgControlsUpdated(const MsgControlsUpdated &other);
            MsgControlsUpdated(const Message &message);
            ~MsgControlsUpdated();
            MsgControlsUpdated &operator =(const MsgControlsUpdated &other);
            bool operator ==(const MsgControlsUpdated &other) const;
            operator Message() const;

            const std::string &device() const;
            bool updated() const;

        private:
            MsgControlsUpdatedPrivate *d;
    };

    class MsgFrameReadyPrivate;
    class VideoFrame;

    class MsgFrameReady: public MsgCommons
    {
        public:
            MsgFrameReady(const std::string &device={},
                          bool isActive=false,
                          uint64_t queryId=0);
            MsgFrameReady(const std::string &device,
                          const VideoFrame &frame,
                          bool isActive=false,
                          uint64_t queryId=0);
            MsgFrameReady(const MsgFrameReady &other);
            MsgFrameReady(const Message &message);
            ~MsgFrameReady();
            MsgFrameReady &operator =(const MsgFrameReady &other);
            bool operator ==(const MsgFrameReady &other) const;
            operator Message() const;

            const std::string &device() const;
            const VideoFrame &frame() const;
            bool isActive() const;

        private:
            MsgFrameReadyPrivate *d;
    };

    class MsgBroadcastPrivate;

    class MsgBroadcast: public MsgCommons
    {
        public:
            MsgBroadcast(const std::string &device={},
                         uint64_t pid=0,
                         uint64_t queryId=0);
            MsgBroadcast(const std::string &device,
                         uint64_t pid,
                         const VideoFrame &frame,
                         uint64_t queryId=0);
            MsgBroadcast(const MsgBroadcast &other);
            MsgBroadcast(const Message &message);
            ~MsgBroadcast();
            MsgBroadcast &operator =(const MsgBroadcast &other);
            bool operator ==(const MsgBroadcast &other) const;
            operator Message() const;

            const std::string &device() const;
            uint64_t pid() const;
            const VideoFrame &frame() const;

        private:
            MsgBroadcastPrivate *d;
    };

    class MsgListenPrivate;

    class MsgListen: public MsgCommons
    {
        public:
            MsgListen(const std::string &device={},
                      uint64_t pid=0,
                      uint64_t queryId=0);
            MsgListen(const MsgListen &other);
            MsgListen(const Message &message);
            ~MsgListen();
            MsgListen &operator =(const MsgListen &other);
            bool operator ==(const MsgListen &other) const;
            operator Message() const;

            const std::string &device() const;
            uint64_t pid() const;

        private:
            MsgListenPrivate *d;
    };
}

#endif // MESSAGECOMMONS_H
