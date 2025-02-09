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

#ifndef MESSAGESERVER_H
#define MESSAGESERVER_H

#include <cstdint>
#include <functional>
#include <vector>

#include "utils.h"

namespace AkVCam
{
    class MessageServerPrivate;
    class Message;

    class MessageServer
    {
        public:
            AKVCAM_SIGNAL(ConnectionClosed, uint64_t clientId)

        public:
            using MessageHandler =
                std::function<bool (uint64_t clientId,
                                    const Message &inMessage,
                                    Message &outMessage)>;

            MessageServer();
            ~MessageServer();

            uint16_t port() const;
            void setPort(uint16_t port);
            bool subscribe(int messageId, MessageHandler messageHandlerFunc);
            bool unsubscribe(int messageId);
            int run();
            void stop();

        private:
            MessageServerPrivate *d;

        friend class MessageServerPrivate;
    };
}

#endif // MESSAGESERVER_H
