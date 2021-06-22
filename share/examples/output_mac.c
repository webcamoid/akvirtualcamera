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
#include <time.h>

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

    // Start the stream.
    proc = popen(cmd, "w");

    // Allocate the frame buffer.
    size_t buffer_size = 3 * width * height;
    char *buffer = malloc(buffer_size);

    // Generate some random noise frames.
    srand(time(0));

    for (int i = 0; i < N_FRAMES; i++) {
        // Write the frame data to the buffer.
        for (size_t byte = 0; byte < buffer_size; byte++)
            buffer[byte] = rand() & 0xff;

        fwrite(buffer, buffer_size, 1, proc);

        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 1e9 / FPS;
        nanosleep(&ts, &ts);
    }

    // Release the frame buffer.
    free(buffer);

    // Stop the stream.
    pclose(proc);

    return 0;
}
