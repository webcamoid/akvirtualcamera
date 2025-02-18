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
            Message(int id);
            Message(int id, uint64_t queryId);
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
            MsgCommons();
            MsgCommons(uint64_t queryId);
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
            MsgStatus();
            MsgStatus(int status);
            MsgStatus(int status, uint64_t queryId);
            MsgStatus(const MsgStatus &other);
            MsgStatus(const Message &message);
            ~MsgStatus();
            MsgStatus &operator =(const MsgStatus &other);
            bool operator ==(const MsgStatus &other) const;
            Message toMessage() const;

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

            MsgClients();
            MsgClients(ClientType clientType);
            MsgClients(ClientType clientType,
                       const std::vector<uint64_t> &clients);
            MsgClients(ClientType clientType,
                       const std::vector<uint64_t> &clients,
                       uint64_t queryId);
            MsgClients(const MsgClients &other);
            MsgClients(const Message &message);
            ~MsgClients();
            MsgClients &operator =(const MsgClients &other);
            bool operator ==(const MsgClients &other) const;
            Message toMessage() const;

            ClientType clientType() const;
            const std::vector<uint64_t> &clients() const;

        private:
            MsgClientsPrivate *d;
    };

    class MsgFrameReadyPrivate;
    class VideoFrame;

    class MsgFrameReady: public MsgCommons
    {
        public:
            MsgFrameReady();
            MsgFrameReady(const std::string &device);
            MsgFrameReady(const std::string &device, bool isActive);
            MsgFrameReady(const std::string &device,
                          bool isActive,
                          uint64_t queryId);
            MsgFrameReady(const std::string &device,
                          const VideoFrame &frame,
                          bool isActive);
            MsgFrameReady(const std::string &device,
                          const VideoFrame &frame,
                          bool isActive,
                          uint64_t queryId);
            MsgFrameReady(const MsgFrameReady &other);
            MsgFrameReady(const Message &message);
            ~MsgFrameReady();
            MsgFrameReady &operator =(const MsgFrameReady &other);
            bool operator ==(const MsgFrameReady &other) const;
            Message toMessage() const;

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
            MsgBroadcast();
            MsgBroadcast(const std::string &device);
            MsgBroadcast(const std::string &device, uint64_t pid);
            MsgBroadcast(const std::string &device,
                         uint64_t pid,
                         uint64_t queryId);
            MsgBroadcast(const std::string &device,
                         uint64_t pid,
                         const VideoFrame &frame);
            MsgBroadcast(const std::string &device,
                         uint64_t pid,
                         const VideoFrame &frame,
                         uint64_t queryId);
            MsgBroadcast(const MsgBroadcast &other);
            MsgBroadcast(const Message &message);
            ~MsgBroadcast();
            MsgBroadcast &operator =(const MsgBroadcast &other);
            bool operator ==(const MsgBroadcast &other) const;
            Message toMessage() const;

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
            MsgListen();
            MsgListen(const std::string &device);
            MsgListen(const std::string &device, uint64_t pid);
            MsgListen(const std::string &device,
                      uint64_t pid,
                      uint64_t queryId);
            MsgListen(const MsgListen &other);
            MsgListen(const Message &message);
            ~MsgListen();
            MsgListen &operator =(const MsgListen &other);
            bool operator ==(const MsgListen &other) const;
            Message toMessage() const;

            const std::string &device() const;
            uint64_t pid() const;

        private:
            MsgListenPrivate *d;
    };
}

#endif // MESSAGECOMMONS_H
