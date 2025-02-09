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

#ifndef COREMEDIAIO_CMIOHARDWARESTREAM_H
#define COREMEDIAIO_CMIOHARDWARESTREAM_H

#include "CMIOHardwareObject.h"
#include "CMIOStreamClock.h"

using CMIOStreamID = CMIOObjectID;

using CMIODeviceStreamQueueAlteredProc = void (*)(CMIOStreamID streamID, void *token, void *refCon);

enum kCMIOStreamProperties
{
    kCMIOStreamPropertyFrameRates      = CFTYPE_FOURCC('n','f', 'r', '#'),
    kCMIOStreamPropertyFrameRateRanges = CFTYPE_FOURCC('f','r', 'r', 'g'),
    kCMIOStreamPropertyDirection       = CFTYPE_FOURCC('s','d', 'i', 'r'),
};

#endif // COREMEDIAIO_CMIOHARDWARESTREAM_H
