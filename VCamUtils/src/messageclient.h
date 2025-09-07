/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef AKVCAMUTILS_MESSAGECLIENT_H
#define AKVCAMUTILS_MESSAGECLIENT_H

#include <cstdint>
#include <functional>
#include <future>
#include <vector>

namespace AkVCam
{
    class MessageClientPrivate;
    class Message;

    class MessageClient
    {
        public:
            using InMessageHandler =
                std::function<bool (Message &inMessage)>;
            using OutMessageHandler =
                std::function<bool (const Message &outMessage)>;

            MessageClient();
            ~MessageClient();

            uint16_t port() const;
            void setPort(uint16_t port);
            static bool isUp(uint16_t port);
            bool send(const Message &inMessage, Message &outMessage) const;
            bool send(const Message &inMessage) const;
            std::future<bool> send(InMessageHandler inData,
                                   OutMessageHandler outData);
            std::future<bool> send(InMessageHandler inData);
            std::future<bool> send(const Message &inMessage,
                                   OutMessageHandler outData);

        private:
            MessageClientPrivate *d;
    };
}

#endif // AKVCAMUTILS_MESSAGECLIENT_H
