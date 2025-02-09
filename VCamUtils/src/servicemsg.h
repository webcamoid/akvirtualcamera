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

#ifndef SERVICEMSG_H
#define SERVICEMSG_H

// Clients listing
#define AKVCAM_SERVICE_MSG_CLIENTS           0x001

// Response messages
#define AKVCAM_SERVICE_MSG_STATUS            0x101
#define AKVCAM_SERVICE_MSG_FRAME_READY       0x102

// Broadcasting messages
#define AKVCAM_SERVICE_MSG_BROADCAST         0x201
#define AKVCAM_SERVICE_MSG_LISTEN            0x202

// Hit messages for updating the cameras in the client side
#define AKVCAM_SERVICE_MSG_UPDATE_DEVICES    0x301
#define AKVCAM_SERVICE_MSG_DEVICES_UPDATED   0x302

// Hit messages for updating the camera controls in the client side
#define AKVCAM_SERVICE_MSG_UPDATE_CONTROLS   0x401
#define AKVCAM_SERVICE_MSG_CONTROLS_UPDATED  0x402

// Hit messages for updating the default picture in the client side
#define AKVCAM_SERVICE_MSG_UPDATE_PICTURE    0x501
#define AKVCAM_SERVICE_MSG_PICTURE_UPDATED   0x502

#endif // SERVICEMSG_H
