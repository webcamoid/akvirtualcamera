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

#ifndef AKVCAMUTILS_LOGGER_H
#define AKVCAMUTILS_LOGGER_H

#include <iostream>

#define AKVCAM_LOGLEVEL_DEFAULT    -1
#define AKVCAM_LOGLEVEL_EMERGENCY   0
#define AKVCAM_LOGLEVEL_FATAL       1
#define AKVCAM_LOGLEVEL_CRITICAL    2
#define AKVCAM_LOGLEVEL_ERROR       3
#define AKVCAM_LOGLEVEL_WARNING     4
#define AKVCAM_LOGLEVEL_NOTICE      5
#define AKVCAM_LOGLEVEL_INFO        6
#define AKVCAM_LOGLEVEL_DEBUG       7

#define AkLog(level, format, ...)    AkVCam::Logger::log(level, __FILE__, __LINE__, false, format, ##__VA_ARGS__)
#define AkLogEmergency(format, ...)  AkLog(AKVCAM_LOGLEVEL_EMERGENCY, format, ##__VA_ARGS__)
#define AkLogFatal(format, ...)      AkLog(AKVCAM_LOGLEVEL_FATAL, format, ##__VA_ARGS__)
#define AkLogCritical(format, ...)   AkLog(AKVCAM_LOGLEVEL_CRITICAL, format, ##__VA_ARGS__)
#define AkLogError(format, ...)      AkLog(AKVCAM_LOGLEVEL_ERROR, format, ##__VA_ARGS__)
#define AkLogWarning(format, ...)    AkLog(AKVCAM_LOGLEVEL_WARNING, format, ##__VA_ARGS__)
#define AkLogNotice(format, ...)     AkLog(AKVCAM_LOGLEVEL_NOTICE, format, ##__VA_ARGS__)
#define AkLogInfo(format, ...)       AkLog(AKVCAM_LOGLEVEL_INFO, format, ##__VA_ARGS__)
#define AkLogDebug(format, ...)      AkLog(AKVCAM_LOGLEVEL_DEBUG, format, ##__VA_ARGS__)

#if defined(__GNUC__) || defined(__clang__)
#   define AkLogFunction()  AkLogDebug("%s", __PRETTY_FUNCTION__)
#elif defined(_MSC_VER)
#   define AkLogFunction()  AkLogDebug("%s", __FUNCSIG__)
#else
#   define AkLogFunction()  AkLogDebug("%s", __FUNCTION__)
#endif

#define AkPrint(level, format, ...)  AkVCam::Logger::log(level, __FILE__, __LINE__, true, format, ##__VA_ARGS__)
#define AkPrintOut(format, ...)      AkPrint(AKVCAM_LOGLEVEL_INFO, format, ##__VA_ARGS__)
#define AkPrintErr(format, ...)      AkPrint(AKVCAM_LOGLEVEL_ERROR, format, ##__VA_ARGS__)

namespace AkVCam
{
    namespace Logger
    {
        std::string context();
        void setContext(const std::string &context);
        std::string logFile();
        void setLogFile(const std::string &fileName);
        int logLevel();
        void setLogLevel(int logLevel);
        size_t bufferSize();
        void setBufferSize(size_t bufferSize);
        void log(int logLevel,
                 const char *file,
                 int line,
                 bool raw,
                 const char *format,
                 ...);
        int levelFromString(const std::string &level);
        std::string levelToString(int level);
    }
}

#endif // AKVCAMUTILS_LOGGER_H
