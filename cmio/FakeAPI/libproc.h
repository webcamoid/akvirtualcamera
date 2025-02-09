/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef LIBPROC_H
#define LIBPROC_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "PlatformUtils/src/utils.h"

int proc_listallpids(void *buffer, size_t buffersize)
{
    auto dir = opendir("/proc");

    if (buffer && buffersize > 0)
        memset(buffer, 0, buffersize);

    if (!dir)
        return 0;

    std::vector<pid_t> pids;

    for (;;) {
        auto entry = readdir(dir);

        if (!entry)
            break;

        if (strncmp(entry->d_name, ".", 256) == 0
            || strncmp(entry->d_name, "..", 256) == 0
            || entry->d_type != DT_DIR) {
            continue;
        }

        auto isNumber = std::all_of(entry->d_name,
                                    entry->d_name + strnlen(entry->d_name, 256),
                                    [] (char c) -> bool {
            return isdigit(c);
        });

        if (!isNumber)
            continue;

        pid_t pid = atoi(entry->d_name);
        char procExePath[4096];
        snprintf(procExePath, 4096, "/proc/%d/exe", pid);

        if (AkVCam::realPath(procExePath).empty())
            continue;

        pids.push_back(pid);
    }

    closedir(dir);

    if (buffer && buffersize > 0)
        memcpy(buffer, pids.data(), std::min(pids.size() * sizeof(pid_t), buffersize));

    return pids.size();
}

inline void proc_pidpath(uint64_t pid, char *path, size_t size)
{
    char procPath[4096];
    snprintf(procPath, 4096, "/proc/%d/exe", pid);
    auto procRealPath = AkVCam::realPath(procPath);
    snprintf(path, size, "%s", procRealPath.c_str());
}

#endif // LIBPROC_H
