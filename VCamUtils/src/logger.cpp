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
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <memory>
#include <mutex>

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

        static const LogLevelStr *byLevel(int logLevel)
        {
            auto level = table();

            for (; level->logLevel != AKVCAM_LOGLEVEL_DEBUG; ++level)
                if (level->logLevel == logLevel)
                    return level;

            return level;
        }
    };

    class LoggerPrivate
    {
        public:
            std::string context;
            std::string logFile;
            std::string fileName;
            int logLevel {AKVCAM_LOGLEVEL_DEFAULT};
            char *logBuffer {nullptr};
            size_t bufferSize {4096};
            FILE *fileStream {nullptr};
            std::mutex logMutex;

            LoggerPrivate()
            {
                this->logBuffer = new char [this->bufferSize];
            }

            ~LoggerPrivate()
            {
                if (this->fileStream)
                    fclose(this->fileStream);

                if (this->logBuffer)
                    delete [] this->logBuffer;
            }

            static std::string header(int logLevel, const char *file, int line);
            static std::string timeStamp();

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

std::string AkVCam::Logger::context()
{
    return loggerPrivate()->context;
}

void AkVCam::Logger::setContext(const std::string &context)
{
    loggerPrivate()->context = context;
}

std::string AkVCam::Logger::logFile()
{
    return loggerPrivate()->logFile;
}

void AkVCam::Logger::setLogFile(const std::string &fileName)
{
    loggerPrivate()->logFile = fileName;
    auto index = fileName.rfind('.');

    if (loggerPrivate()->fileStream) {
        fclose(loggerPrivate()->fileStream);
        loggerPrivate()->fileStream = nullptr;
    }

    if (index == fileName.npos) {
        loggerPrivate()->fileName = fileName + "-" + LoggerPrivate::timeStamp();
    } else {
        auto fname = fileName.substr(0, index);
        auto extension = fileName.substr(index + 1);
        loggerPrivate()->fileName = fname + "-" + LoggerPrivate::timeStamp() + "." + extension;
    }

    if (!loggerPrivate()->fileStream) {
        loggerPrivate()->fileStream = fopen(loggerPrivate()->fileName.c_str(), "a");

        if (loggerPrivate()->fileStream)
            setvbuf(loggerPrivate()->fileStream, NULL, _IONBF, 0);
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

size_t AkVCam::Logger::bufferSize()
{
    return loggerPrivate()->bufferSize;
}

void AkVCam::Logger::setBufferSize(size_t bufferSize)
{
    loggerPrivate()->bufferSize = bufferSize;

    if (loggerPrivate()->logBuffer) {
        delete [] loggerPrivate()->logBuffer;
        loggerPrivate()->logBuffer = nullptr;
    }

    if (loggerPrivate()->bufferSize > 0)
        loggerPrivate()->logBuffer = new char [loggerPrivate()->bufferSize];
}

void AkVCam::Logger::log(int logLevel,
                         const char *file,
                         int line,
                         bool raw,
                         const char *format,
                         ...)
{
    if (!raw && logLevel > loggerPrivate()->logLevel)
        return;

    std::lock_guard<std::mutex> lock(loggerPrivate()->logMutex);
    auto logBuffer = loggerPrivate()->logBuffer;

    if (!logBuffer)
        return;

    auto bufferSize = loggerPrivate()->bufferSize;

    if (bufferSize < 1)
        return;

    std::string log;
    auto header = LoggerPrivate::header(logLevel, file, line);

    if (!raw)
        log += header;

    va_list args;
    va_start(args, format);
    vsnprintf(logBuffer, bufferSize, format, args);
    va_end(args);

    log += logBuffer;
    log += '\n';

    if (loggerPrivate()->fileStream) {
        std::string line;

        if (raw)
            line = header;

        fwrite(line.c_str(), 1, line.size(), loggerPrivate()->fileStream);
        fflush(loggerPrivate()->fileStream);
    }

    auto out = logLevel == AKVCAM_LOGLEVEL_INFO? stdout: stderr;
    fwrite(log.data(),
           1,
           log.size(),
           out);
    fflush(out);
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

std::string AkVCam::LoggerPrivate::header(int logLevel,
                                          const char *file,
                                          int line)
{
    static std::mutex headerMutex;
    auto now = std::chrono::system_clock::now();
    auto nowMSecs =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    auto time = std::chrono::system_clock::to_time_t(now);

    char timestamp[32];
    {
        std::lock_guard<std::mutex> lock(headerMutex);
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
                "[%s.%03lld, %llu, %s, %s (%d)] %s: ",
                timestamp,
                nowMSecs.count() % 1000,
                LoggerPrivate::threadID(),
                loggerPrivate()->context.c_str(),
                file,
                line,
                LogLevelStr::byLevel(logLevel)->str);
#else
    snprintf(buffer,
             sizeof(buffer),
             "[%s.%03lld, %llu, %s, %s (%d)] %s: ",
             timestamp,
             nowMSecs.count() % 1000,
             LoggerPrivate::threadID(),
             loggerPrivate()->context.c_str(),
             file,
             line,
             LogLevelStr::byLevel(logLevel)->str);
#endif

    return {buffer};
}

std::string AkVCam::LoggerPrivate::timeStamp()
{
    static std::mutex tsMutex;
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    char buffer[16];

    {
        std::lock_guard<std::mutex> lock(tsMutex);

#ifdef _WIN32
        struct tm timeInfo;
        localtime_s(&timeInfo, &time);
#else
        auto timeInfo = *std::localtime(&time);
#endif

        strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &timeInfo);
    }

    return std::string(buffer);
}
