/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
 *
 * akvirtualcamera is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either vcam_version 3 of the License, or
 * (at your option) any later vcam_version.
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

#ifndef CAPI_H
#define CAPI_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
    #define CAPI_EXPORT __declspec(dllexport)
#else
    #define CAPI_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Virtual camera driver ID
CAPI_EXPORT void vcam_id(char *id, size_t *id_len);

// Virtual camera driver version
CAPI_EXPORT void vcam_version(int *major, int *minor, int *patch);

// Open the virtual camera driver
CAPI_EXPORT void *vcam_open();

// Close the virtual camera
CAPI_EXPORT void vcam_close(void *vcam);

// List devices.
CAPI_EXPORT int vcam_devices(void *vcam, char *devs, size_t *buffer_size);

// Add a new device.
CAPI_EXPORT int vcam_add_device(void *vcam,
                                const char *description,
                                char *device_id,
                                size_t buffer_size);

// Remove a device.
CAPI_EXPORT int vcam_remove_device(void *vcam, const char *device_id);

// Remove all devices.
CAPI_EXPORT int vcam_remove_devices(void *vcam);

// Get device description.
CAPI_EXPORT int vcam_description(void *vcam,
                                 const char *device_id,
                                 char *device_description,
                                 size_t *buffer_size);

// Set device description.
CAPI_EXPORT int vcam_set_description(void *vcam,
                                     const char *device_id,
                                     const char *description);

// List supported input formats.
CAPI_EXPORT int vcam_supported_input_formats(void *vcam,
                                             char *formats,
                                             size_t *buffer_size);

// List supported output formats.
CAPI_EXPORT int vcam_supported_output_formats(void *vcam,
                                              char *formats,
                                              size_t *buffer_size);

// Default input format for the device.
CAPI_EXPORT int vcam_default_input_format(void *vcam,
                                          char *format,
                                          size_t *buffer_size);

// Default output format for the device.
CAPI_EXPORT int vcam_default_output_format(void *vcam,
                                           char *format,
                                           size_t *buffer_size);

// Get device output format.
CAPI_EXPORT int vcam_format(void *vcam,
                            const char *device_id,
                            int index,
                            char *format,
                            size_t *format_bfsz,
                            int *width,
                            int *height,
                            int *fps_num,
                            int *fps_den);

// Add a new device format
CAPI_EXPORT int vcam_add_format(void *vcam,
                                const char *device_id,
                                const char *format,
                                int width,
                                int height,
                                int fps_num,
                                int fps_den,
                                int *index);

// Remove device format
CAPI_EXPORT int vcam_remove_format(void *vcam,
                                   const char *device_id,
                                   int index);

// Remove all device formats
CAPI_EXPORT int vcam_remove_formats(void *vcam, const char *device_id);

// Update devices.
CAPI_EXPORT int vcam_update(void *vcam);

// Create devices from a setting file.
CAPI_EXPORT int vcam_load(void *vcam, const char *settings_ini);

// Start video streaming to the virtual camera
CAPI_EXPORT int vcam_stream_start(void *vcam, const char *device_id);

// Send a video to the virtual camera
CAPI_EXPORT int vcam_stream_send(void *vcam,
                                 const char *device_id,
                                 const char *format,
                                 int width,
                                 int height,
                                 const char **data,
                                 size_t *line_size);

// Stop video streaming
CAPI_EXPORT int vcam_stream_stop(void *vcam, const char *device_id);

// Set afunction for listening global events.
typedef void (* vcam_event_fn)(void *context, const char *event);
CAPI_EXPORT int vcam_set_event_listener(void *vcam,
                                        void *context,
                                        vcam_event_fn event_listener);

// Get device control
CAPI_EXPORT int vcam_control(void *vcam,
                             const char *device_id,
                             int index,
                             char *name,
                             size_t *name_bfsz,
                             char *description,
                             size_t *description_bfsz,
                             char *type,
                             size_t *type_bfsz,
                             int *min,
                             int *max,
                             int *step,
                             int *value,
                             int *default_value,
                             char *menu,
                             size_t *menu_bfsz);

// Set a device control values.
CAPI_EXPORT int vcam_set_controls(void *vcam,
                                  const char *device_id,
                                  const char **controls,
                                  int *values,
                                  size_t n_controls);

// Placeholder picture to show when no streaming.
CAPI_EXPORT int vcam_picture(void *vcam, char *file_path, size_t *buffer_size);

// Set placeholder picture.
CAPI_EXPORT int vcam_set_picture(void *vcam, const char *file_path);

// Show current debugging level.
CAPI_EXPORT int vcam_loglevel(void *vcam, int *level);

// Set debugging level.
CAPI_EXPORT int vcam_set_loglevel(void *vcam, int level);

// List clients using the camera.
CAPI_EXPORT int vcam_clients(void *vcam, uint64_t *pids, size_t npids);

// List clients using the camera.
CAPI_EXPORT int vcam_client_path(void *vcam,
                                 uint64_t pid,
                                 char *path,
                                 size_t *buffer_size);

#ifdef __cplusplus
}
#endif

#endif // CAPI_H
