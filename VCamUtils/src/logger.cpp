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

#include <chrono>
#include <ctime>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <pthread.h>
#else
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "logger.h"
#include "utils.h"

namespace AkVCam
{
    struct LogLevelStr
    {
        int logLevel;
        const char *str;

        LogLevelStr(int logLevel, const char *str):
            logLevel(logLevel),
            str(str)
        {
        }

        static const LogLevelStr *table()
        {
            static const LogLevelStr akvcamLoggerStrTable[] = {
                {AKVCAM_LOGLEVEL_DEFAULT  , "default"  },
                {AKVCAM_LOGLEVEL_EMERGENCY, "emergency"},
                {AKVCAM_LOGLEVEL_FATAL    , "fatal"    },
                {AKVCAM_LOGLEVEL_CRITICAL , "critical" },
                {AKVCAM_LOGLEVEL_ERROR    , "error"    },
                {AKVCAM_LOGLEVEL_WARNING  , "warning"  },
                {AKVCAM_LOGLEVEL_NOTICE   , "notice"   },
                {AKVCAM_LOGLEVEL_INFO     , "info"     },
                {AKVCAM_LOGLEVEL_DEBUG    , "debug"    },
            };

            return akvcamLoggerStrTable;
        }
    };

    class LoggerPrivate
    {
        public:
            std::string logFile;
            std::string fileName;
            int logLevel {AKVCAM_LOGLEVEL_DEFAULT};
            std::fstream stream;

            inline static uint64_t threadID()
            {
#ifdef _WIN32
                return GetCurrentThreadId();
#elif defined(__APPLE__)
                uint64_t tid;
                pthread_threadid_np(NULL, &tid);

                return tid;
#else
                return static_cast<uint64_t>(gettid());
#endif
            }
    };

    LoggerPrivate *loggerPrivate()
    {
        static LoggerPrivate logger;

        return &logger;
    }
}

std::string AkVCam::Logger::logFile()
{
    return loggerPrivate()->logFile;
}

void AkVCam::Logger::setLogFile(const std::string &fileName)
{
    loggerPrivate()->logFile = fileName;
    auto index = fileName.rfind('.');

    if (index == fileName.npos) {
        loggerPrivate()->fileName = fileName + "-" + timeStamp();
    } else {
        std::string fname = fileName.substr(0, index);
        std::string extension = fileName.substr(index + 1);
        loggerPrivate()->fileName = fname + "-" + timeStamp() + "." + extension;
    }
}

int AkVCam::Logger::logLevel()
{
    return loggerPrivate()->logLevel;
}

void AkVCam::Logger::setLogLevel(int logLevel)
{
    loggerPrivate()->logLevel = logLevel;
}

std::string AkVCam::Logger::header(int logLevel,
                                   const std::string &file,
                                   int line)
{
    static std::mutex mutex;
    auto now = std::chrono::system_clock::now();
    auto nowMSecs =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    auto time = std::chrono::system_clock::to_time_t(now);

    char timestamp[32];
    {
        std::lock_guard<std::mutex> lock(mutex);
#ifdef _WIN32
        struct tm timeInfo;
        localtime_s(&timeInfo, &time);
#else
        auto timeInfo = *std::localtime(&time);
#endif
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeInfo);
    }

    thread_local char buffer[256];

#ifdef _WIN32
    _snprintf_s(buffer,
                sizeof(buffer),
                "%s.%03lld, %llu, %s (%d)] %s: ",
                timestamp,
                nowMSecs.count() % 1000,
                LoggerPrivate::threadID(),
                file.c_str(),
                line,
                levelToString(logLevel).c_str());
#else
    snprintf(buffer,
             sizeof(buffer),
             "%s.%03lld, %llu, %s (%d)] %s: ",
             timestamp,
             nowMSecs.count() % 1000,
             LoggerPrivate::threadID(),
             file.c_str(),
             line,
             levelToString(logLevel).c_str());
#endif

    return {buffer};
}

std::ostream &AkVCam::Logger::log(int logLevel)
{
    static std::ostream dummy(nullptr);

    if (logLevel > loggerPrivate()->logLevel)
        return dummy;

    if (loggerPrivate()->fileName.empty())
        return logLevel == AKVCAM_LOGLEVEL_INFO? std::cout: std::cerr;

    if (!loggerPrivate()->stream.is_open())
        loggerPrivate()->stream.open(loggerPrivate()->fileName,
                                     std::ios_base::out | std::ios_base::app);

    if (!loggerPrivate()->stream.is_open())
        return logLevel == AKVCAM_LOGLEVEL_INFO? std::cout: std::cerr;

    return loggerPrivate()->stream;
}

int AkVCam::Logger::levelFromString(const std::string &level)
{
    auto lvl = LogLevelStr::table();

    for (; lvl->logLevel != AKVCAM_LOGLEVEL_DEBUG; ++lvl)
        if (lvl->str == level)
            return lvl->logLevel;

    return lvl->logLevel;
}

std::string AkVCam::Logger::levelToString(int level)
{
    auto lvl = LogLevelStr::table();

    for (; lvl->logLevel != AKVCAM_LOGLEVEL_DEBUG; ++lvl)
        if (lvl->logLevel == level)
            return {lvl->str};

    return {lvl->str};
}
