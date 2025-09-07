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

#ifndef AKVCAMUTILS_VIDEOFRAME_H
#define AKVCAMUTILS_VIDEOFRAME_H

#include <string>
#include <memory>
#include <vector>

#include "videoframetypes.h"
#include "videoformat.h"

namespace AkVCam
{
    using Rgb = uint32_t;

    class VideoFramePrivate;
    class VideoFormat;

    class VideoFrame
    {
        public:
            VideoFrame();
            VideoFrame(const std::string &fileName);
            VideoFrame(const VideoFormat &format, bool initialized=false);
            VideoFrame(const VideoFrame &other);
            VideoFrame &operator =(const VideoFrame &other);
            operator bool() const;
            ~VideoFrame();

            bool load(const std::string &fileName);
            VideoFormat format() const;
            size_t size() const;
            size_t planes() const;
            size_t planeSize(int plane) const;
            size_t pixelSize(int plane) const;
            size_t lineSize(int plane) const;
            size_t bytesUsed(int plane) const;
            size_t widthDiv(int plane) const;
            size_t heightDiv(int plane) const;
            const uint8_t *constData() const;
            uint8_t *data();
            const uint8_t *constPlane(int plane) const;
            uint8_t *plane(int plane);
            const uint8_t *constLine(int plane, int y) const;
            uint8_t *line(int plane, int y);
            VideoFrame copy(int x,
                            int y,
                            int width,
                            int height) const;

            template <typename T>
            inline T pixel(int plane, int x, int y) const
            {
                auto line = reinterpret_cast<const T *>(this->constLine(plane, y));

                return line[x >> this->widthDiv(plane)];
            }

            template <typename T>
            inline void setPixel(int plane, int x, int y, T value)
            {
                auto line = reinterpret_cast<T *>(this->line(plane, y));
                line[x >> this->widthDiv(plane)] = value;
            }

            template <typename T>
            inline void fill(int plane, T value)
            {
                int width = this->format().width() >> this->widthDiv(plane);
                auto line = new T [width];

                for (int x = 0; x < width; x++)
                    line[x] = value;

                size_t lineSize = width * sizeof(T);

                for (int y = 0; y < this->format().height(); y++)
                    memcpy(this->line(plane, y), line, lineSize);

                delete [] line;
            }

            template <typename T>
            inline void fill(T value)
            {
                for (size_t plane = 0; plane < this->planes(); plane++)
                    this->fill(plane, value);
            }

            void fillRgb(Rgb color);

        private:
            VideoFramePrivate *d;
    };
}

#endif // AKVCAMUTILS_VIDEOFRAME_H
