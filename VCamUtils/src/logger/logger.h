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

#include "../utils.h"

#define AKVCAM_LOGLEVEL_DEFAULT    -1
#define AKVCAM_LOGLEVEL_EMERGENCY   0
#define AKVCAM_LOGLEVEL_FATAL       1
#define AKVCAM_LOGLEVEL_CRITICAL    2
#define AKVCAM_LOGLEVEL_ERROR       3
#define AKVCAM_LOGLEVEL_WARNING     4
#define AKVCAM_LOGLEVEL_NOTICE      5
#define AKVCAM_LOGLEVEL_INFO        6
#define AKVCAM_LOGLEVEL_DEBUG       7

#define AkLog(level)     AkVCam::Logger::log(level) << AkVCam::Logger::header(level, __FILE__, __LINE__)
#define AkLogEmergency() AkLog(AKVCAM_LOGLEVEL_EMERGENCY)
#define AkLogFatal()     AkLog(AKVCAM_LOGLEVEL_FATAL)
#define AkLogCritical()  AkLog(AKVCAM_LOGLEVEL_CRITICAL)
#define AkLogError()     AkLog(AKVCAM_LOGLEVEL_ERROR)
#define AkLogWarning()   AkLog(AKVCAM_LOGLEVEL_WARNING)
#define AkLogNotice()    AkLog(AKVCAM_LOGLEVEL_NOTICE)
#define AkLogInfo()      AkLog(AKVCAM_LOGLEVEL_INFO)
#define AkLogDebug()     AkLog(AKVCAM_LOGLEVEL_DEBUG)

#if defined(__GNUC__) || defined(__clang__)
#   define AkLogFunction()  AkLogDebug() << __PRETTY_FUNCTION__ << std::endl
#elif defined(_MSC_VER)
#   define AkLogFunction()  AkLogDebug() << __FUNCSIG__ << std::endl
#else
#   define AkLogFunction()  AkLogDebug() << __FUNCTION__ << "()" << std::endl
#endif

namespace AkVCam
{
    namespace Logger
    {
        std::string logFile();
        void setLogFile(const std::string &fileName);
        int logLevel();
        void setLogLevel(int logLevel);
        std::string header(int logLevel, const std::string file, int line);
        std::ostream &log(int logLevel);
        int levelFromString(const std::string &level);
        std::string levelToString(int level);
    }
}

#endif // AKVCAMUTILS_LOGGER_H
