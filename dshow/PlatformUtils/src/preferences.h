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

#include <memory>
#include <string>
#include <vector>
#include <strmif.h>

namespace AkVCam
{
    class VideoFormat;

    namespace Preferences
    {
        void write(const std::string &key, const std::string &value);
        void write(const std::string &key, int value);
        void write(const std::string &key, double value);
        void write(const std::string &key, std::vector<std::string> &value);
        std::string readString(const std::string &key,
                               const std::string &defaultValue={});
        int readInt(const std::string &key, int defaultValue=0);
        double readDouble(const std::string &key, double defaultValue=0.0);
        bool readBool(const std::string &key, bool defaultValue=false);
        std::vector<CLSID> listRegisteredCameras(HINSTANCE hinstDLL);
        void deleteKey(const std::string &key);
        void move(const std::string &keyFrom, const std::string &keyTo);
        std::string addDevice(const std::string &description);
        std::string addCamera(const std::string &description,
                              const std::vector<VideoFormat> &formats);
        std::string addCamera(const std::string &path,
                              const std::string &description,
                              const std::vector<VideoFormat> &formats);
        void removeCamera(const std::string &path);
        size_t camerasCount();
        std::string createDevicePath();
        int cameraFromCLSID(const CLSID &clsid);
        int cameraFromPath(const std::string &path);
        bool cameraExists(const std::string &path);
        std::string cameraDescription(size_t cameraIndex);
        void cameraSetDescription(size_t cameraIndex,
                                  const std::string &description);
        std::string cameraPath(size_t cameraIndex);
        size_t formatsCount(size_t cameraIndex);
        VideoFormat cameraFormat(size_t cameraIndex, size_t formatIndex);
        std::vector<VideoFormat> cameraFormats(size_t cameraIndex);
        void cameraSetFormats(size_t cameraIndex,
                              const std::vector<VideoFormat> &formats);
        void cameraAddFormat(size_t cameraIndex,
                             const VideoFormat &format,
                             int index);
        void cameraRemoveFormat(size_t cameraIndex, int index);
        int cameraControlValue(size_t cameraIndex,
                               const std::string &key);
        void cameraSetControlValue(size_t cameraIndex,
                                   const std::string &key,
                                   int value);
        std::string picture();
        void setPicture(const std::string &picture);
        int logLevel();
        void setLogLevel(int logLevel);
    }
}

#endif // PREFERENCES_H
