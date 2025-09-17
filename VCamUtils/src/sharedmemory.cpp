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

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#endif

#include "sharedmemory.h"
#include "logger.h"

namespace AkVCam
{
    class SharedMemoryPrivate
    {
        public:
#ifdef _WIN32
            HANDLE m_sharedHandle {nullptr};
            HANDLE m_mutex {nullptr};
#else
            int m_sharedHandle {-1};
            sem_t *m_mutex {nullptr};
#endif
            std::string m_name;
            void *m_buffer {nullptr};
            size_t m_pageSize {0};
            SharedMemory::OpenMode m_mode {SharedMemory::OpenModeRead};
            bool m_isOpen {false};
    };
}

AkVCam::SharedMemory::SharedMemory()
{
    this->d = new SharedMemoryPrivate;
}

AkVCam::SharedMemory::SharedMemory(const SharedMemory &other)
{
    this->d = new SharedMemoryPrivate;
    this->d->m_name = other.d->m_name;
    this->d->m_pageSize = other.d->m_pageSize;
    this->d->m_mode = other.d->m_mode;

    if (other.d->m_isOpen)
        this->open(other.d->m_pageSize, other.d->m_mode);
}

AkVCam::SharedMemory::~SharedMemory()
{
    this->close();
    delete this->d;
}

AkVCam::SharedMemory &AkVCam::SharedMemory::operator =(const SharedMemory &other)
{
    if (this != &other) {
        this->close();
        this->d->m_name = other.d->m_name;
        this->d->m_buffer = nullptr;
        this->d->m_pageSize = other.d->m_pageSize;
        this->d->m_mode = other.d->m_mode;
        this->d->m_isOpen = false;

        if (other.d->m_isOpen)
            this->open(other.d->m_pageSize, other.d->m_mode);
    }

    return *this;
}

std::string AkVCam::SharedMemory::name() const
{
    return this->d->m_name;
}

void AkVCam::SharedMemory::setName(const std::string &name)
{
    this->d->m_name = name;
}

bool AkVCam::SharedMemory::open(size_t pageSize, OpenMode mode)
{
    if (this->d->m_isOpen)
        return false;

    if (this->d->m_name.empty())
        return false;

    auto mutexName = this->d->m_name + "_mutex";

#ifdef _WIN32
    // Create mutex
    this->d->m_mutex = CreateMutexA(nullptr, FALSE, mutexName.c_str());

    if (!this->d->m_mutex) {
        AkLogError() << "Error creating mutex ("
                     << this->d->m_name
                     << ") with error 0x"
                     << std::hex << GetLastError()
                     << std::endl;

        return false;
    }

    // Open or create shared memory
    if (mode == OpenModeRead) {
        this->d->m_sharedHandle =
                OpenFileMappingA(FILE_MAP_ALL_ACCESS,
                                 FALSE,
                                 this->d->m_name.c_str());
    } else {
        if (pageSize < 1) {
            CloseHandle(this->d->m_mutex);
            this->d->m_mutex = nullptr;

            return false;
        }

        this->d->m_sharedHandle =
                CreateFileMappingA(INVALID_HANDLE_VALUE,
                                   nullptr,
                                   PAGE_READWRITE,
                                   0,
                                   DWORD(pageSize),
                                   this->d->m_name.c_str());
    }

    if (!this->d->m_sharedHandle) {
        AkLogError() << "Error opening shared memory ("
                     << this->d->m_name
                     << ") with error 0x"
                     << std::hex << GetLastError()
                     << std::endl;
        CloseHandle(this->d->m_mutex);
        this->d->m_mutex = nullptr;

        return false;
    }

    // Map shared memory
    this->d->m_buffer = MapViewOfFile(this->d->m_sharedHandle,
                                      FILE_MAP_ALL_ACCESS,
                                      0,
                                      0,
                                      pageSize);
#else
    // Create semaphore
    this->d->m_mutex = sem_open(mutexName.c_str(), O_CREAT, 0644, 1);

    if (this->d->m_mutex == SEM_FAILED) {
        AkLogError() << "Error creating semaphore ("
                     << mutexName
                     << ") with error "
                     << errno
                     << std::endl;

        return false;
    }

    // Open or create shared memory
    if (mode == OpenModeRead) {
        this->d->m_sharedHandle =
                shm_open(this->d->m_name.c_str(), O_RDWR, 0644);
    } else {
        if (pageSize < 1) {
            sem_close(this->d->m_mutex);
            sem_unlink(mutexName.c_str());
            this->d->m_mutex = nullptr;

            return false;
        }

        this->d->m_sharedHandle = shm_open(this->d->m_name.c_str(),
                                           O_CREAT | O_RDWR,
                                           0644);

        if (this->d->m_sharedHandle != -1
            && ftruncate(this->d->m_sharedHandle, pageSize) == -1) {
            AkLogError() << "Error setting shared memory size ("
                         << this->d->m_name
                         << ") with error "
                         << errno
                         << std::endl;
            ::close(this->d->m_sharedHandle);
            this->d->m_sharedHandle = -1;
            sem_close(this->d->m_mutex);
            sem_unlink(mutexName.c_str());
            this->d->m_mutex = nullptr;

            return false;
        }
    }

    if (this->d->m_sharedHandle == -1) {
        AkLogError() << "Error opening shared memory ("
                     << this->d->m_name
                     << ") with error "
                     << errno
                     << std::endl;
        sem_close(this->d->m_mutex);
        sem_unlink(mutexName.c_str());
        this->d->m_mutex = nullptr;

        return false;
    }

    // Map shared memory
    this->d->m_buffer = mmap(nullptr,
                             pageSize,
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED,
                             this->d->m_sharedHandle,
                             0);

    if (this->d->m_buffer == MAP_FAILED) {
        AkLogError() << "Error mapping shared memory ("
                     << this->d->m_name
                     << ") with error "
                     << errno
                     << std::endl;
        ::close(this->d->m_sharedHandle);
        this->d->m_sharedHandle = -1;
        sem_close(this->d->m_mutex);
        sem_unlink(mutexName.c_str());
        this->d->m_mutex = nullptr;

        return false;
    }
#endif

    this->d->m_pageSize = pageSize;
    this->d->m_mode = mode;
    this->d->m_isOpen = true;

    return true;
}

bool AkVCam::SharedMemory::isOpen() const
{
    return this->d->m_isOpen;
}

size_t AkVCam::SharedMemory::pageSize() const
{
    return this->d->m_pageSize;
}

AkVCam::SharedMemory::OpenMode AkVCam::SharedMemory::mode() const
{
    return this->d->m_mode;
}

void *AkVCam::SharedMemory::lock(int timeout)
{
    if (!this->d->m_mutex)
        return nullptr;

#ifdef _WIN32
    DWORD waitResult = WaitForSingleObject(this->d->m_mutex,
                                           !timeout? INFINITE: DWORD(timeout));

    if (waitResult == WAIT_FAILED || waitResult == WAIT_TIMEOUT)
        return nullptr;
#else
    if (timeout == 0) {
        if (sem_wait(this->d->m_mutex) != 0)
            return nullptr;
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout / 1000;
        ts.tv_nsec += (timeout % 1000) * 1000000;

        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        if (sem_timedwait(this->d->m_mutex, &ts) != 0)
            return nullptr;
    }
#endif

    return this->d->m_buffer;
}

void AkVCam::SharedMemory::unlock()
{
    if (this->d->m_mutex) {
#ifdef _WIN32
        ReleaseMutex(this->d->m_mutex);
#else
        sem_post(this->d->m_mutex);
#endif
    }
}

void AkVCam::SharedMemory::close()
{
    if (this->d->m_buffer) {
#ifdef _WIN32
        UnmapViewOfFile(this->d->m_buffer);
#else
        munmap(this->d->m_buffer, this->d->m_pageSize);
#endif

        this->d->m_buffer = nullptr;
    }

#ifdef _WIN32
    if (this->d->m_sharedHandle) {
        CloseHandle(this->d->m_sharedHandle);
        this->d->m_sharedHandle = nullptr;
    }
#else
    if (this->d->m_sharedHandle != -1) {
        close(this->d->m_sharedHandle);
        this->d->m_sharedHandle = -1;

        if (this->d->m_mode != OpenModeRead)
            shm_unlink(this->d->m_name.c_str());
    }
#endif

    if (this->d->m_mutex) {
#ifdef _WIN32
        CloseHandle(this->d->m_mutex);
#else
        sem_close(this->d->m_mutex);
        sem_unlink((this->d->m_name + "_mutex").c_str());
#endif

        this->d->m_mutex = nullptr;
    }

    this->d->m_pageSize = 0;
    this->d->m_mode = OpenModeRead;
    this->d->m_isOpen = false;
}
