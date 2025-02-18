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

#ifndef STREAM_H
#define STREAM_H

#include "VCamUtils/src/fraction.h"
#include "VCamUtils/src/videoframetypes.h"
#include "VCamUtils/src/ipcbridge.h"
#include "object.h"
#include "queue.h"

namespace AkVCam
{
    class StreamPrivate;
    class Device;
    class Stream;
    class VideoFrame;
    using StreamPtr = std::shared_ptr<Stream>;
    using SampleBufferQueue = Queue<CMSampleBufferRef>;
    using SampleBufferQueuePtr = QueuePtr<CMSampleBufferRef>;

    class Stream: public Object
    {
        public:
            Stream(bool registerObject=false, Object *m_parent=nullptr);
            Stream(const Stream &other) = delete;
            ~Stream();

            OSStatus createObject();
            OSStatus registerObject(bool regist=true);
            Device *device() const;
            void setDevice(Device *device);
            void setBridge(IpcBridgePtr bridge);
            void setFormats(const std::vector<VideoFormat> &formats);
            void setFormat(const VideoFormat &format);
            void setFrameRate(const Fraction &frameRate);
            bool start();
            void stop();
            bool running();

            void frameReady(const VideoFrame &frame, bool isActive);
            void setPicture(const std::string &picture);
            void setHorizontalMirror(bool horizontalMirror);
            void setVerticalMirror(bool verticalMirror);
            void setScaling(Scaling scaling);
            void setAspectRatio(AspectRatio aspectRatio);
            void setSwapRgb(bool swap);

            // Stream Interface
            OSStatus copyBufferQueue(CMIODeviceStreamQueueAlteredProc queueAlteredProc,
                                     void *queueAlteredRefCon,
                                     CMSimpleQueueRef *queue);
            OSStatus deckPlay();
            OSStatus deckStop();
            OSStatus deckJog(SInt32 speed);
            OSStatus deckCueTo(Float64 frameNumber, Boolean playOnCue);

        private:
            StreamPrivate *d;

            friend class StreamPrivate;
    };
}

#endif // STREAM_H
