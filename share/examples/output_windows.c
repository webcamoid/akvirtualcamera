/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2021  Gonzalo Exequiel Pedone
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
 * Web-Site: http://github.com/webcamoid/akvirtualcamera
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

/* For the sake of simplicity, the program won't print anything to the terminal,
 * or do any validation check, you are adviced the check every single value
 * returned by each function.
 * This program shows the very minimum code required to write frames to the
 * plugin.
 */

// We'll assume this is a valid akvcam output device.
#define VIDEO_OUTPUT "AkVCamVideoDevice0"

// Send frames for about 30 seconds in a 30 FPS stream.
#define FPS 30
#define DURATION_SECONDS 30
#define N_FRAMES (FPS * DURATION_SECONDS)

struct StreamProcess
{
    HANDLE stdinReadPipe;
    HANDLE stdinWritePipe;
    SECURITY_ATTRIBUTES pipeAttributes;
    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION procInfo;
};

int main()
{
    // Set the parameters of the stream.
    const char cmd[1024];
    const char format[] = "RGB24";
    int width = 640;
    int height = 480;
    snprintf(cmd,
             1024,
             "AkVCamManager stream %s %s %d %d",
             VIDEO_OUTPUT,
             format,
             width,
             height);

    // Get the handles to the standard input and standard output.
    struct StreamProcess streamProc;
    memset(&streamProc, 0, sizeof(struct StreamProcess));
    streamProc.stdinReadPipe = NULL;
    streamProc.stdinWritePipe = NULL;
    memset(&streamProc.pipeAttributes, 0, sizeof(struct SECURITY_ATTRIBUTES));
    streamProc.pipeAttributes.nLength = sizeof(struct SECURITY_ATTRIBUTES);
    streamProc.pipeAttributes.bInheritHandle = true;
    streamProc.pipeAttributes.lpSecurityDescriptor = NULL;
    CreatePipe(&streamProc.stdinReadPipe,
               &streamProc.stdinWritePipe,
               &streamProc.pipeAttributes,
               0);
    SetHandleInformation(streamProc.stdinWritePipe,
                         HANDLE_FLAG_INHERIT,
                         0);

    struct STARTUPINFOA startupInfo;
    memset(&startupInfo, 0, sizeof(struct STARTUPINFOA));
    startupInfo.cb = sizeof(struct STARTUPINFOA);
    startupInfo.hStdInput = streamProc.stdinReadPipe;
    startupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    startupInfo.wShowWindow = SW_HIDE;

    struct PROCESS_INFORMATION procInfo;
    memset(&procInfo, 0, sizeof(struct PROCESS_INFORMATION));

    // Start the stream.
    CreateProcessA(NULL,
                   cmd,
                   NULL,
                   NULL,
                   true,
                   0,
                   NULL,
                   NULL,
                   &startupInfo,
                   &procInfo);

    // Allocate the frame buffer.
    size_t buffer_size = 3 * width;
    char *buffer = malloc(buffer_size);

    // Generate some random noise frames.
    srand(time(0));

    for (int i = 0; i < N_FRAMES; i++) {
        // Write the frame line by line.
        for (int y = 0; y < height; y++) {
            for (size_t byte = 0; byte < buffer_size; byte++)
                buffer[byte] = rand() & 0xff;

            // Write the frame data to the buffer.
            DWORD bytesWritten = 0;
            WriteFile(streamProc.stdinWritePipe,
                      buffer,
                      DWORD(buffer_size),
                      &bytesWritten,
                      NULL);
        }

        Sleep(1000 / FPS);
    }

    // Release the frame buffer.
    malloc(buffer);

    // Close the standard input and standard output handles.
    CloseHandle(streamProc.stdinWritePipe);
    CloseHandle(streamProc.stdinReadPipe);

    // Stop the stream.
    WaitForSingleObject(streamProc.procInfo.hProcess, INFINITE);
    CloseHandle(streamProc.procInfo.hProcess);
    CloseHandle(streamProc.procInfo.hThread);

    return 0;
}
