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
    };

    class TeeStreambuf: public std::streambuf
    {
        public:
            TeeStreambuf(const std::vector<std::streambuf *> &buffers={}):
                m_buffers(buffers)
            {
            }

            TeeStreambuf(const TeeStreambuf &other):
                m_buffers(other.m_buffers)
            {

            }

            TeeStreambuf &operator =(const TeeStreambuf &other)
            {
                if (this != &other)
                    this->m_buffers = other.m_buffers;

                return *this;
            }

        protected:
            int overflow(int c) override
            {
                if (c != EOF)
                    for (auto &buffer: this->m_buffers)
                        buffer->sputc(c);

                return c;
            }

            int sync() override
            {
                for (auto &buffer: this->m_buffers)
                    buffer->pubsync();

                return 0;
            }

        private:
            std::vector<std::streambuf *> m_buffers;
    };

    class LoggerPrivate
    {
        public:
            std::string context;
            std::string logFile;
            std::string fileName;
            int logLevel {AKVCAM_LOGLEVEL_DEFAULT};
            std::fstream stream;
            TeeStreambuf teeOutBuf;
            TeeStreambuf teeErrBuf;
            std::shared_ptr<std::ostream> teeOut;
            std::shared_ptr<std::ostream> teeErr;

            LoggerPrivate()
            {
            }

            ~LoggerPrivate()
            {
            }

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

    if (loggerPrivate()->stream.is_open())
        loggerPrivate()->stream.close();

    if (index == fileName.npos) {
        loggerPrivate()->fileName = fileName + "-" + LoggerPrivate::timeStamp();
    } else {
        std::string fname = fileName.substr(0, index);
        std::string extension = fileName.substr(index + 1);
        loggerPrivate()->fileName = fname + "-" + LoggerPrivate::timeStamp() + "." + extension;
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
                "[%s.%03lld, %llu, %s, %s (%d)] %s: ",
                timestamp,
                nowMSecs.count() % 1000,
                LoggerPrivate::threadID(),
                loggerPrivate()->context.c_str(),
                file.c_str(),
                line,
                levelToString(logLevel).c_str());
#else
    snprintf(buffer,
             sizeof(buffer),
             "[%s.%03lld, %llu, %s, %s (%d)] %s: ",
             timestamp,
             nowMSecs.count() % 1000,
             LoggerPrivate::threadID(),
             loggerPrivate()->context.c_str(),
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

    if (!loggerPrivate()->stream.is_open()) {
        loggerPrivate()->stream.open(loggerPrivate()->fileName,
                                     std::ios_base::out | std::ios_base::app);

        loggerPrivate()->teeOutBuf = {{loggerPrivate()->stream.rdbuf(), std::cout.rdbuf()}};
        loggerPrivate()->teeErrBuf = {{loggerPrivate()->stream.rdbuf(), std::cerr.rdbuf()}};

        loggerPrivate()->teeOut = std::make_shared<std::ostream>(&loggerPrivate()->teeOutBuf);
        loggerPrivate()->teeErr = std::make_shared<std::ostream>(&loggerPrivate()->teeErrBuf);
    }

    if (!loggerPrivate()->stream.is_open())
        return logLevel == AKVCAM_LOGLEVEL_INFO? std::cout: std::cerr;

    if (logLevel == AKVCAM_LOGLEVEL_INFO) {
        loggerPrivate()->teeOut->flush();

        return *loggerPrivate()->teeOut;
    }

    loggerPrivate()->teeErr->flush();

    return *loggerPrivate()->teeErr;
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

std::string AkVCam::LoggerPrivate::timeStamp()
{
    static std::mutex mutex;
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    char buffer[16];

    {
        std::lock_guard<std::mutex> lock(mutex);

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
