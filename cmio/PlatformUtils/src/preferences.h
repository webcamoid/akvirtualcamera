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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <string>
#include <vector>
#include <CoreFoundation/CoreFoundation.h>

namespace AkVCam
{
    class VideoFormat;

    namespace Preferences
    {
        std::vector<std::string> keys();
        void write(const std::string &key,
                   const std::shared_ptr<CFTypeRef> &value);
        void write(const std::string &key, const std::string &value);
        void write(const std::string &key, const std::wstring &value);
        void write(const std::string &key, int value);
        void write(const std::string &key, double value);
        std::shared_ptr<CFTypeRef> read(const std::string &key);
        std::string readString(const std::string &key,
                               const std::string &defaultValue={});
        std::wstring readWString(const std::string &key,
                                 const std::wstring &defaultValue={});
        int readInt(const std::string &key, int defaultValue=0);
        double readDouble(const std::string &key, double defaultValue=0.0);
        void deleteKey(const std::string &key);
        void deleteAllKeys(const std::string &key);
        void move(const std::string &keyFrom, const std::string &keyTo);
        void moveAll(const std::string &keyFrom, const std::string &keyTo);
        void sync();
        std::string addCamera(const std::wstring &description,
                              const std::vector<VideoFormat> &formats);
        std::string addCamera(const std::string &path,
                              const std::wstring &description,
                              const std::vector<VideoFormat> &formats);
        void removeCamera(const std::string &path);
        size_t camerasCount();
        std::string createDevicePath();
        int cameraFromPath(const std::string &path);
        bool cameraExists(const std::string &path);
        std::wstring cameraDescription(size_t cameraIndex);
        std::string cameraPath(size_t cameraIndex);
        size_t formatsCount(size_t cameraIndex);
        VideoFormat cameraFormat(size_t cameraIndex, size_t formatIndex);
        std::vector<VideoFormat> cameraFormats(size_t cameraIndex);
    }
}

#endif // PREFERENCES_H
