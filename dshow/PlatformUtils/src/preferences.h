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
        bool write(const std::string &key,
                   const std::string &value,
                   bool global=false);
        bool write(const std::string &key, int value, bool global=false);
        bool write(const std::string &key, double value, bool global=false);
        bool write(const std::string &key,
                   std::vector<std::string> &value,
                   bool global=false);
        std::string readString(const std::string &key,
                               const std::string &defaultValue={},
                               bool global=false);
        int readInt(const std::string &key,
                    int defaultValue=0,
                    bool global=false);
        double readDouble(const std::string &key,
                          double defaultValue=0.0,
                          bool global=false);
        bool readBool(const std::string &key,
                      bool defaultValue=false,
                      bool global=false);
        bool deleteKey(const std::string &key, bool global=false);
        bool move(const std::string &keyFrom,
                  const std::string &keyTo,
                  bool global=false);
        std::string addDevice(const std::string &description,
                              const std::string &deviceId);
        std::string addCamera(const std::string &description,
                              const std::vector<VideoFormat> &formats);
        std::string addCamera(const std::string &path,
                              const std::string &description,
                              const std::vector<VideoFormat> &formats);
        bool removeCamera(const std::string &path);
        size_t camerasCount();
        bool idDeviceIdTaken(const std::string &deviceId);
        std::string createDevicePath();
        int cameraFromCLSID(const CLSID &clsid);
        int cameraFromPath(const std::string &path);
        bool cameraExists(const std::string &path);
        std::string cameraDescription(size_t cameraIndex);
        bool cameraSetDescription(size_t cameraIndex,
                                  const std::string &description);
        std::string cameraPath(size_t cameraIndex);
        size_t formatsCount(size_t cameraIndex);
        VideoFormat cameraFormat(size_t cameraIndex, size_t formatIndex);
        std::vector<VideoFormat> cameraFormats(size_t cameraIndex);
        bool cameraSetFormats(size_t cameraIndex,
                              const std::vector<VideoFormat> &formats);
        bool cameraAddFormat(size_t cameraIndex,
                             const VideoFormat &format,
                             int index);
        bool cameraRemoveFormat(size_t cameraIndex, int index);
        int cameraControlValue(size_t cameraIndex,
                               const std::string &key);
        bool cameraSetControlValue(size_t cameraIndex,
                                   const std::string &key,
                                   int value);
        std::string picture();
        bool setPicture(const std::string &picture);
        int logLevel();
        bool setLogLevel(int logLevel);
    }
}

#endif // PREFERENCES_H
