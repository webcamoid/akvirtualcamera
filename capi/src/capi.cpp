/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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

#include <algorithm>
#include <cstring>

#include "capi.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/fraction.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/settings.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"
#include "VCamUtils/src/logger.h"

class VCamAPI
{
    public:
        using StringVector = std::vector<std::string>;
        using StringMatrix = std::vector<StringVector>;
        using VideoFormatMatrix = std::vector<std::vector<AkVCam::VideoFormat>>;

        AkVCam::IpcBridge m_bridge;
        vcam_event_fn m_eventListener {nullptr};
        void *m_context {nullptr};

        ~VCamAPI();
        static void devicesChanged(void *context, const std::vector<std::string> &);
        static void pictureChanged(void *context, const std::string &);
        void loadGenerals(AkVCam::Settings &settings);
        StringMatrix matrixCombine(const StringMatrix &matrix) const;
        void matrixCombineP(const StringMatrix &matrix,
                            size_t index,
                            StringVector combined,
                            StringMatrix &combinations) const;
        std::vector<AkVCam::VideoFormat> readFormat(AkVCam::Settings &settings) const;
        VideoFormatMatrix readFormats(AkVCam::Settings &settings) const;
        std::vector<AkVCam::VideoFormat> readDeviceFormats(AkVCam::Settings &settings,
                                                           const VideoFormatMatrix &availableFormats) const;
        void createDevice(AkVCam::Settings &settings,
                          const VideoFormatMatrix &availableFormats);
        void createDevices(AkVCam::Settings &settings,
                           const VideoFormatMatrix &availableFormats);
};

struct ControlTypeStr
{
    AkVCam::ControlType type {AkVCam::ControlTypeUnknown};
    const char *str {nullptr};

    ControlTypeStr(AkVCam::ControlType type, const char *str):
        type(type),
        str(str)
    {

    }

    inline static const ControlTypeStr *table()
    {
        static const ControlTypeStr akVCamCAPITypeStr[] = {
            {AkVCam::ControlTypeInteger, "Integer"},
            {AkVCam::ControlTypeBoolean, "Boolean"},
            {AkVCam::ControlTypeMenu   , "Menu"   },
            {AkVCam::ControlTypeUnknown, ""       }
        };

        return akVCamCAPITypeStr;
    }

    inline static const char *toString(AkVCam::ControlType type)
    {
        auto control = table();

        for (; control->type != AkVCam::ControlTypeUnknown; ++control)
            if (control->type == type)
                return control->str;

        return control->str;
    }

    inline static AkVCam::ControlType fromString(const char *str)
    {
        auto control = table();

        for (; control->type != AkVCam::ControlTypeUnknown; ++control)
            if (strcmp(control->str, str) == 0)
                return control->type;

        return control->type;
    }
};

CAPI_EXPORT void vcam_id(char *id, size_t *id_len)
{
    // Get the length of COMMONS_APPNAME including the null terminator
    size_t len = strlen(COMMONS_APPNAME) + 1;

    // Set the buffer size in buffer_size if not null
    if (id_len != nullptr) {
        *id_len = len;

        // Copy COMMONS_APPNAME to id if id is not null
        if (id != nullptr)
            std::copy_n(COMMONS_APPNAME, len, id);
    }
}

CAPI_EXPORT void vcam_version(int *major, int *minor, int *patch)
{
    if (major)
        *major = COMMONS_VER_MAJ;

    if (minor)
        *minor = COMMONS_VER_MIN;

    if (patch)
        *patch = COMMONS_VER_PAT;
}

CAPI_EXPORT void *vcam_open()
{
    auto vcamApi = new VCamAPI;

    auto logFile = vcamApi->m_bridge.logPath("AkVCamAPI");
    AkLogInfo() << "Sending debug output to " << logFile << std::endl;
    AkVCam::Logger::setLogFile(logFile);

    return vcamApi;
}

CAPI_EXPORT void vcam_close(void *vcam)
{
    if (vcam)
        delete reinterpret_cast<VCamAPI *>(vcam);
}

CAPI_EXPORT int vcam_devices(void *vcam, char *devs, size_t *buffer_size)
{
    // Validate buffer_size
    if (!buffer_size)
        return -EINVAL;

    // Cast vcam to VCamAPI
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    // Get devices from the bridge
    auto devices = vcamApi->m_bridge.devices();

    // Calculate required buffer size
    size_t totalSize = 1; // Final \x0

    for (const auto &device : devices)
        totalSize += device.size() + 1; // String + \x0 separator

    *buffer_size = totalSize;

    // Return device count if devs is null, or if no devices
    if (!devs || devices.empty())
        return static_cast<int>(devices.size());

    // Copy devices to buffer
    size_t offset = 0;
    size_t devicesToCopy = 0;

    for (const auto &device: devices) {
        // Ensure we don't write past the provided buffer_size
        if (offset + device.size() + 1 > *buffer_size)
            break;

        std::memcpy(devs + offset, device.c_str(), device.size());
        offset += device.size();
        devs[offset++] = '\x0'; // Add null terminator after each string
        devicesToCopy++;
    }

    // Add final \x0 terminator
    devs[offset] = '\x0';

    return static_cast<int>(devicesToCopy);
}

CAPI_EXPORT int vcam_add_device(void *vcam,
                                const char *description,
                                char *device_id,
                                size_t buffer_size)
{
    auto vcamApi = reinterpret_cast<VCamAPI*>(vcam);

    if (!vcamApi)
        return -EINVAL;

    // Basic parameter validation
    if (!description || !*description || !device_id || buffer_size == 0)
        return -EINVAL;

    // Common configuration
    std::string generatedId;
    bool useExistingId = strlen(device_id) > 0;

    // Privileged execution path (requires root)
    if (AkVCam::needsRoot("add-device")) {
        auto managerPath = AkVCam::locateManagerPath();

        if (managerPath.empty())
            return -ENOENT;

        // Get current devices before adding
        auto devicesBefore = vcamApi->m_bridge.devices();

        // Prepare arguments for sudo
        std::vector<std::string> args;

        if (useExistingId)
            args = {managerPath, "add-device", "-i", device_id, description};
        else
            args = {managerPath, "add-device", description};

        // Execute command with elevated privileges
        int result = AkVCam::sudo(args);

        if (result < 0)
            return result;

        // Get devices after adding and find the new ID
        auto devicesAfter = vcamApi->m_bridge.devices();

        for (const auto &device: devicesAfter)
            if (std::find(devicesBefore.begin(),
                          devicesBefore.end(),
                          device) == devicesBefore.end()) {
                // Assume the new device is the one not present before
                generatedId = device;

                break;
            }

        if (generatedId.empty())
            return -ENOENT; // No new device found
    } else {
        // Unprivileged execution (root not required)
        if (useExistingId)
            generatedId = vcamApi->m_bridge.addDevice(description, device_id);
        else
            generatedId = vcamApi->m_bridge.addDevice(description);
    }

    // Copy generated ID to output buffer with null termination
    std::copy_n(generatedId.c_str(), buffer_size - 1, device_id);
    device_id[buffer_size - 1] = '\0';

    return 0; // Success
}

CAPI_EXPORT int vcam_remove_device(void *vcam, const char *device_id)
{
    // Validate device_id parameter
    if (!device_id || !*device_id)
        return -EINVAL;

    // Root-required execution path
    if (AkVCam::needsRoot("remove-device")) {
        auto manager = AkVCam::locateManagerPath();

        if (manager.empty())
            return -ENOENT;

        return AkVCam::sudo({manager, "remove-device", device_id});
    }

    // Normal execution path
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    // Check if device exists
    const auto &devices = vcamApi->m_bridge.devices();

    if (std::find(devices.begin(), devices.end(), device_id) == devices.end())
        return -ENODEV;

    // Remove the device
    vcamApi->m_bridge.removeDevice(device_id);

    return 0;
}

CAPI_EXPORT int vcam_remove_devices(void *vcam)
{
    if (AkVCam::needsRoot("remove-devices")) {
        auto manager = AkVCam::locateManagerPath();

        if (manager.empty())
            return -ENOENT;

        return AkVCam::sudo({manager, "remove-devices"});
    }

    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    for (auto &device: vcamApi->m_bridge.devices())
        vcamApi->m_bridge.removeDevice(device);

    return 0;
}

CAPI_EXPORT int vcam_description(void *vcam,
                                 const char *device_id,
                                 char *device_description,
                                 size_t *buffer_size)
{
    // Validate buffer_size
    if (!buffer_size)
        return -EINVAL;

    // Cast vcam to VCamAPI
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    // Validate device_id
    if (!device_id)
        return -EINVAL;

    // Get devices and check if device_id exists
    auto deviceList = vcamApi->m_bridge.devices();
    std::string deviceIdStr(device_id);
    auto it = std::find(deviceList.begin(), deviceList.end(), deviceIdStr);

    if (it == deviceList.end())
        return -EINVAL;

    // Get device description
    auto description = vcamApi->m_bridge.description(deviceIdStr);
    *buffer_size = description.size() + 1; // Include null terminator

    // Return 0 if device_description is null
    if (!device_description)
        return 0;

    // Copy description to buffer
    size_t copySize = std::min<size_t>(description.size(), *buffer_size - 1);
    std::memcpy(device_description, description.c_str(), copySize);
    device_description[copySize] = '\x0';

    return static_cast<int>(copySize);
}

CAPI_EXPORT int vcam_set_description(void *vcam,
                                     const char *device_id,
                                     const char *description)
{
    if (AkVCam::needsRoot("set-description")) {
        auto manager = AkVCam::locateManagerPath();

        if (manager.empty())
            return -ENOENT;

        return AkVCam::sudo({manager, "set-description", device_id, description});
    }

    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    auto devices = vcamApi->m_bridge.devices();
    auto it = std::find(devices.begin(), devices.end(), device_id);

    if (it == devices.end())
        return -ENODEV;

    vcamApi->m_bridge.setDescription(device_id, description);

    return 0;
}

CAPI_EXPORT int vcam_supported_input_formats(void *vcam,
                                             char *formats,
                                             size_t *buffer_size)
{
    // Validate buffer_size
    if (!buffer_size)
        return -EINVAL;

    // Cast vcam to VCamAPI
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    // Get and convert supported pixel formats
    std::vector<std::string> formatStrings;
    auto formatList =
            vcamApi->m_bridge.supportedPixelFormats(AkVCam::IpcBridge::StreamType_Input);

    for (const auto &format: formatList)
        formatStrings.push_back(AkVCam::pixelFormatToCommonString(format));

    // Calculate required buffer size
    size_t totalSize = 1; // Final \x0

    for (const auto &format: formatStrings)
        totalSize += format.size() + 1; // String + \x0 separator

    *buffer_size = totalSize;

    // Return format count if formats is null, or if no formats
    if (!formats || formatStrings.empty())
        return static_cast<int>(formatStrings.size());

    // Copy formats to buffer
    size_t offset = 0;
    size_t formatsToCopy = 0;

    for (const auto &format: formatStrings) {
        // Ensure we don't write past the provided buffer_size
        if (offset + format.size() + 1 > *buffer_size)
            break;

        std::memcpy(formats + offset, format.c_str(), format.size());
        offset += format.size();
        formats[offset++] = '\x0'; // Add null terminator after each string
        formatsToCopy++;
    }

    // Add final \x0 terminator
    formats[offset] = '\x0';

    return static_cast<int>(formatsToCopy);
}

CAPI_EXPORT int vcam_supported_output_formats(void *vcam,
                                              char *formats,
                                              size_t *buffer_size)
{
    // Validate buffer_size
    if (!buffer_size)
        return -EINVAL;

    // Cast vcam to VCamAPI
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    // Get and convert supported pixel formats
    std::vector<std::string> formatStrings;
    auto formatList =
            vcamApi->m_bridge.supportedPixelFormats(AkVCam::IpcBridge::StreamType_Output);

    for (const auto &format: formatList)
        formatStrings.push_back(AkVCam::pixelFormatToCommonString(format));

    // Calculate required buffer size
    size_t totalSize = 1; // Final \x0

    for (const auto &format: formatStrings)
        totalSize += format.size() + 1; // String + \x0 separator

    *buffer_size = totalSize;

    // Return format count if formats is null, or if no formats
    if (!formats || formatStrings.empty())
        return static_cast<int>(formatStrings.size());

    // Copy formats to buffer
    size_t offset = 0;
    size_t formatsToCopy = 0;

    for (const auto &format: formatStrings) {
        // Ensure we don't write past the provided buffer_size
        if (offset + format.size() + 1 > *buffer_size)
            break;

        std::memcpy(formats + offset, format.c_str(), format.size());
        offset += format.size();
        formats[offset++] = '\x0'; // Add null terminator after each string
        formatsToCopy++;
    }

    // Add final \x0 terminator
    formats[offset] = '\x0';

    return static_cast<int>(formatsToCopy);
}

CAPI_EXPORT int vcam_default_input_format(void *vcam,
                                          char *format,
                                          size_t *buffer_size)
{
    // Validate buffer_size
    if (!buffer_size)
        return -EINVAL;

    // Cast vcam to VCamAPI
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    // Get default pixel format and convert to string
    auto defaultFormat =
            vcamApi->m_bridge.defaultPixelFormat(AkVCam::IpcBridge::StreamType_Input);
    auto formatString = AkVCam::pixelFormatToCommonString(defaultFormat);
    *buffer_size = formatString.size() + 1; // Include null terminator

    // Return 0 if format is null
    if (!format)
        return 0;

    // Copy format to buffer
    size_t copySize = std::min<size_t>(formatString.size(), *buffer_size - 1);
    std::memcpy(format, formatString.c_str(), copySize);
    format[copySize] = '\x0';

    return static_cast<int>(copySize);
}

CAPI_EXPORT int vcam_default_output_format(void *vcam,
                                           char *format,
                                           size_t *buffer_size)
{
    // Validate buffer_size
    if (!buffer_size)
        return -EINVAL;

    // Cast vcam to VCamAPI
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    // Get default pixel format and convert to string
    auto defaultFormat =
            vcamApi->m_bridge.defaultPixelFormat(AkVCam::IpcBridge::StreamType_Output);
    auto formatString = AkVCam::pixelFormatToCommonString(defaultFormat);
    *buffer_size = formatString.size() + 1; // Include null terminator

    // Return 0 if format is null
    if (!format)
        return 0;

    // Copy format to buffer
    size_t copySize = std::min<size_t>(formatString.size(), *buffer_size - 1);
    std::memcpy(format, formatString.c_str(), copySize);
    format[copySize] = '\x0';

    return static_cast<int>(copySize);
}

CAPI_EXPORT int vcam_format(void *vcam,
                            const char *device_id,
                            int index,
                            char *format,
                            size_t *format_bfsz,
                            int *width,
                            int *height,
                            int *fps_num,
                            int *fps_den)
{
    // Validate vcam
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    // Validate device_id
    if (!device_id)
        return -EINVAL;

    // Check if device_id exists
    auto deviceList = vcamApi->m_bridge.devices();
    std::string deviceIdStr(device_id);

    if (std::find(deviceList.begin(),
                  deviceList.end(),
                  deviceIdStr) == deviceList.end())
        return -EINVAL;

    // Get formats for the device
    auto formatList = vcamApi->m_bridge.formats(deviceIdStr);

    if (formatList.empty()) {
        if (format_bfsz)
            *format_bfsz = 0;

        return 0;
    }

    // Validate index
    if (index < 0 || static_cast<size_t>(index) >= formatList.size())
        return -EINVAL;

    // Check if all output parameters are null
    if (!format && !format_bfsz && !width && !height && !fps_num && !fps_den)
        return -EINVAL;

    // Get format at index
    const auto &selectedFormat = formatList[index];

    // Handle format string
    auto formatString =
            AkVCam::pixelFormatToCommonString(selectedFormat.format());

    if (format_bfsz)
        *format_bfsz = formatString.size() + 1; // Include null terminator

    // Copy format string if not null
    if (format && format_bfsz) {
        size_t copySize = std::min<size_t>(formatString.size(), *format_bfsz - 1);
        std::memcpy(format, formatString.c_str(), copySize);
        format[copySize] = '\x0';
    }

    // Copy dimensions and frame rate if not null
    if (width)
        *width = selectedFormat.width();

    if (height)
        *height = selectedFormat.height();

    auto fps = selectedFormat.fps();

    if (fps_num)
        *fps_num = static_cast<int>(fps.num());

    if (fps_den)
        *fps_den = static_cast<int>(fps.den());

    return static_cast<int>(formatList.size());
}

CAPI_EXPORT int vcam_add_format(void *vcam,
                                const char *device_id,
                                const char *format,
                                int width,
                                int height,
                                int fps_num,
                                int fps_den,
                                int *index)
{
    if (!device_id
        || !*device_id
        || !format
        || !*format
        || width < 1
        || height < 1
        || fps_num == 0
        || fps_den == 0) {
        return -EINVAL;
    }

    auto fps = AkVCam::Fraction(fps_num, fps_den);

    if (AkVCam::needsRoot("add-format")) {
        auto manager = AkVCam::locateManagerPath();

        if (manager.empty())
            return -ENOENT;

        if (index)
            return AkVCam::sudo({manager, "add-format", "-i",
                                 std::to_string(*index),
                                 device_id,
                                 format,
                                 std::to_string(width),
                                 std::to_string(height),
                                 fps.toString()});

        return AkVCam::sudo({manager,
                             "add-format",
                             device_id,
                             format,
                             std::to_string(width),
                             std::to_string(height),
                             fps.toString()});
    }

    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    auto devices = vcamApi->m_bridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), device_id);

    if (dit == devices.end())
        return -ENODEV;

    auto fmt = AkVCam::pixelFormatFromCommonString(format);

    auto formats =
            vcamApi->m_bridge.supportedPixelFormats(AkVCam::IpcBridge::StreamType_Output);
    auto fit = std::find(formats.begin(), formats.end(), fmt);

    if (fit == formats.end())
        return -EINVAL;

    if (index)
        vcamApi->m_bridge.addFormat(device_id,
                                    AkVCam::VideoFormat(fmt,
                                                        width,
                                                        height,
                                                        {fps}),
                                    *index);
    else
        vcamApi->m_bridge.addFormat(device_id,
                                    AkVCam::VideoFormat(fmt,
                                                        width,
                                                        height,
                                                        {fps}));

    return 0;
}

CAPI_EXPORT int vcam_remove_format(void *vcam,
                                   const char *device_id,
                                   int index)
{
    if (!device_id || !*device_id)
        return -EINVAL;

    if (index < 0)
        return -ERANGE;

    if (AkVCam::needsRoot("remove-format")) {
        auto manager = AkVCam::locateManagerPath();

        if (manager.empty())
            return -ENOENT;

        return AkVCam::sudo({manager,
                             "remove-format",
                             device_id,
                             std::to_string(index)});
    }

    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    auto devices = vcamApi->m_bridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), device_id);

    if (dit == devices.end())
        return -ENODEV;

    auto formats = vcamApi->m_bridge.formats(device_id);

    if (static_cast<size_t>(index) >= formats.size())
        return -ERANGE;

    vcamApi->m_bridge.removeFormat(device_id, index);

    return 0;
}

CAPI_EXPORT int vcam_remove_formats(void *vcam, const char *device_id)
{
    if (!device_id || !*device_id)
        return -EINVAL;

    if (AkVCam::needsRoot("remove-formats")) {
        auto manager = AkVCam::locateManagerPath();

        if (manager.empty())
            return -ENOENT;

        return AkVCam::sudo({manager, "remove-formats", device_id});
    }

    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    auto devices = vcamApi->m_bridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), device_id);

    if (dit == devices.end())
        return -ENODEV;

    vcamApi->m_bridge.setFormats(device_id, {});

    return 0;
}

CAPI_EXPORT int vcam_update(void *vcam)
{
    if (AkVCam::needsRoot("update")) {
        auto manager = AkVCam::locateManagerPath();

        if (manager.empty())
            return -ENOENT;

        return AkVCam::sudo({manager, "update"});
    }

    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    vcamApi->m_bridge.updateDevices();

    return 0;
}

CAPI_EXPORT int vcam_load(void *vcam, const char *settings_ini)
{
    if (!settings_ini || !*settings_ini)
        return -EINVAL;

    if (AkVCam::needsRoot("load")) {
        auto manager = AkVCam::locateManagerPath();

        if (manager.empty())
            return -ENOENT;

        return AkVCam::sudo({manager, "load", settings_ini});
    }

    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    AkVCam::Settings settings;

    if (!settings.load(settings_ini))
        return -EIO;

    vcamApi->loadGenerals(settings);
    auto devices = vcamApi->m_bridge.devices();

    for (auto &device: devices)
        vcamApi->m_bridge.removeDevice(device);

    vcamApi->createDevices(settings, vcamApi->readFormats(settings));

    return 0;
}

CAPI_EXPORT int vcam_stream_start(void *vcam, const char *device_id)
{
    // Validate vcam
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    // Start device stream
    if (!vcamApi->m_bridge.deviceStart(AkVCam::IpcBridge::StreamType_Output,
                                       {device_id}))
        return -EINVAL;

    return 0;
}

CAPI_EXPORT int vcam_stream_send(void *vcam,
                                 const char *device_id,
                                 const char *format,
                                 int width,
                                 int height,
                                 const char **data,
                                 size_t *line_size)
{
    // Validate vcam and device_id
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi || !device_id)
        return -EINVAL;

    // Check if device_id exists
    auto deviceList = vcamApi->m_bridge.devices();
    std::string deviceIdStr(device_id);

    if (std::find(deviceList.begin(),
                  deviceList.end(),
                  deviceIdStr) == deviceList.end())
        return -EINVAL;

    // Validate format
    if (!format)
        return -EINVAL;

    // Convert and validate format
    auto fourCC = AkVCam::pixelFormatFromCommonString(format);

    if (fourCC == 0)
        return -EINVAL;

    // Check if format is supported
    auto formatList =
            vcamApi->m_bridge.supportedPixelFormats(AkVCam::IpcBridge::StreamType_Output);

    if (std::find(formatList.begin(),
                  formatList.end(),
                  fourCC) == formatList.end())
        return -EINVAL;

    // Validate dimensions and pointers
    if (width < 1 || height < 1 || !data || !line_size)
        return -EINVAL;

    // Create video format and frame
    AkVCam::VideoFrame frame({fourCC, width, height, {30, 1}});

    // Copy data to frame for each plane
    for (size_t plane = 0; plane < frame.planes(); ++plane) {
        if (!data[plane] || line_size[plane] < 1)
            continue;

        size_t bytesPerLine = frame.lineSize(plane);
        size_t copySize = std::min<size_t>(bytesPerLine, line_size[plane]);

        for (int y = 0; y < height; ++y) {
            auto line = frame.line(plane, y);
            auto srcLine = data[plane] + y * line_size[plane];
            std::memcpy(line, srcLine, copySize);
        }
    }

    // Write frame to device
    vcamApi->m_bridge.write(deviceIdStr, frame);

    return 0;
}

CAPI_EXPORT int vcam_stream_stop(void *vcam, const char *device_id)
{
    // Validate vcam
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

     // Stop device stream
    vcamApi->m_bridge.deviceStop({device_id});

    return 0;
}

CAPI_EXPORT int vcam_set_event_listener(void *vcam,
                                        void *context,
                                        vcam_event_fn event_listener)
{
    // Cast api to VCamAPI
    if (!vcam)
        return -EINVAL;

    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    // If event_listener is nullptr, disconnect existing listeners
    if (!event_listener) {
        if (vcamApi->m_eventListener) {
            vcamApi->m_bridge.disconnectDevicesChanged(vcamApi, &VCamAPI::devicesChanged);
            vcamApi->m_bridge.disconnectPictureChanged(vcamApi, &VCamAPI::pictureChanged);
            vcamApi->m_eventListener = nullptr;
            vcamApi->m_context = nullptr;
        }

        return 0;
    }

    // Disconnect previous listeners if they exist
    if (vcamApi->m_eventListener) {
        vcamApi->m_bridge.disconnectDevicesChanged(vcamApi, &VCamAPI::devicesChanged);
        vcamApi->m_bridge.disconnectPictureChanged(vcamApi, &VCamAPI::pictureChanged);
    }

    // Store the new event listener and context
    vcamApi->m_eventListener = event_listener;
    vcamApi->m_context = context;

    // Connect the event listeners
    vcamApi->m_bridge.connectDevicesChanged(vcamApi, &VCamAPI::devicesChanged);
    vcamApi->m_bridge.connectPictureChanged(vcamApi, &VCamAPI::pictureChanged);

    return 0;
}

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
                             size_t *menu_bfsz)
{
    // Validate vcam and device_id
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi || !device_id)
        return -EINVAL;

    // Check if device_id exists
    auto deviceList = vcamApi->m_bridge.devices();

    if (std::find(deviceList.begin(), deviceList.end(), device_id) == deviceList.end())
        return -EINVAL;

    // Get controls and validate index
    auto controlList = vcamApi->m_bridge.controls(device_id);

    if (index < 0 || static_cast<size_t>(index) >= controlList.size())
        return -EINVAL;

    // Select control at index
    const auto &selectedControl = controlList[index];

    // Handle name and name_bfsz
    if (name_bfsz) {
        *name_bfsz = selectedControl.id.size() + 1; // Include null terminator

        if (name && *name_bfsz) {
            std::copy_n(selectedControl.id.c_str(), *name_bfsz, name);
            name[*name_bfsz - 1] = '\x0';
        }
    }

    // Handle description and description_bfsz
    if (description_bfsz) {
        *description_bfsz = selectedControl.description.size() + 1; // Include null terminator

        if (description && *description_bfsz) {
            std::copy_n(selectedControl.description.c_str(), *description_bfsz, description);
            description[*description_bfsz - 1] = '\x0';
        }
    }

    // Handle type and type_bfsz
    auto typeString = ControlTypeStr::toString(selectedControl.type);

    if (type_bfsz) {
        *type_bfsz = typeString ? strlen(typeString) + 1 : 1; // Include null terminator

        if (type && *type_bfsz) {
            std::copy_n(typeString? typeString: "", *type_bfsz, type);
            type[*type_bfsz - 1] = '\x0';
        }
    }

    // Copy numeric values
    if (min)
        *min = selectedControl.minimum;

    if (max)
        *max = selectedControl.maximum;

    if (step)
        *step = selectedControl.step;

    if (value)
        *value = selectedControl.value;

    if (default_value)
        *default_value = selectedControl.defaultValue;

    // Handle menu
    if (menu_bfsz) {
        size_t requiredSize = 1; // Final null terminator

        for (const auto &item : selectedControl.menu)
            requiredSize += item.size() + 1; // Item + null terminator

            *menu_bfsz = requiredSize;

        if (menu && *menu_bfsz) {
            size_t offset = 0;

            for (const auto &item: selectedControl.menu) {
                size_t copySize = std::min<size_t>(item.size(), *menu_bfsz - offset - 1);

                if (copySize == 0)
                    break;

                std::copy_n(item.c_str(), copySize, menu + offset);
                offset += copySize;
                menu[offset++] = '\x0';
            }

            menu[offset] = '\x0'; // Final null terminator
        }
    }

    return static_cast<int>(controlList.size());
}

CAPI_EXPORT int vcam_set_controls(void *vcam,
                                  const char *device_id,
                                  const char **controls,
                                  int *values,
                                  size_t n_controls)
{
    // Validate pointers
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi || !device_id || !controls || !values)
        return -EINVAL;

    // Check if device_id exists
    auto deviceList = vcamApi->m_bridge.devices();

    if (std::find(deviceList.begin(),
                  deviceList.end(),
                  device_id) == deviceList.end())
        return -EINVAL;

    // Return 0 if no controls to set
    if (n_controls == 0)
        return 0;

    // Build controls map
    std::map<std::string, int> controlMap;

    for (size_t i = 0; i < n_controls; ++i)
        if (controls[i])
            controlMap[controls[i]] = values[i];

    // Apply controls
    vcamApi->m_bridge.setControls(device_id, controlMap);

    return 0;
}

CAPI_EXPORT int vcam_picture(void *vcam, char *file_path, size_t *buffer_size)
{
    // Validate pointers
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi || !buffer_size)
        return -EINVAL;

    // Get picture file path
    auto picturePath = vcamApi->m_bridge.picture();
    *buffer_size = picturePath.size() + 1; // Include null terminator

    // Copy path if buffer is provided
    if (file_path && *buffer_size) {
        std::copy_n(picturePath.c_str(), *buffer_size, file_path);
        file_path[*buffer_size - 1] = '\x0';
    }

    return 0;
}

CAPI_EXPORT int vcam_set_picture(void *vcam, const char *file_path)
{
    if (!file_path || !*file_path)
        return -EINVAL;

    if (AkVCam::needsRoot("set-picture")) {
        auto manager = AkVCam::locateManagerPath();

        if (manager.empty())
            return -ENOENT;

        return AkVCam::sudo({manager, "set-picture", file_path});
    }

    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    vcamApi->m_bridge.setPicture(file_path);

    return 0;
}

CAPI_EXPORT int vcam_loglevel(void *vcam, int *level)
{
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi || !level)
        return -EINVAL;

    *level = vcamApi->m_bridge.logLevel();

    return 0;
}

CAPI_EXPORT int vcam_set_loglevel(void *vcam, int level)
{
    if (AkVCam::needsRoot("set-loglevel")) {
        auto manager = AkVCam::locateManagerPath();

        if (manager.empty())
            return -ENOENT;

        return AkVCam::sudo({manager, "set-loglevel", std::to_string(level)});
    }

    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    vcamApi->m_bridge.setLogLevel(level);

    return 0;
}

CAPI_EXPORT int vcam_clients(void *vcam, uint64_t *pids, size_t npids)
{
    // Validate vcam
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi)
        return -EINVAL;

    // Get client PIDs
    auto clientPids = vcamApi->m_bridge.clientsPids();

    // Copy PIDs if buffer is provided
    if (pids && npids > 0) {
        size_t copyCount = std::min<size_t>(npids, clientPids.size());
        std::memcpy(pids, clientPids.data(), copyCount * sizeof(uint64_t));

        return static_cast<int>(copyCount);
    }

    return static_cast<int>(clientPids.size());
}

CAPI_EXPORT int vcam_client_path(void *vcam,
                                 uint64_t pid,
                                 char *path,
                                 size_t *buffer_size)
{
    // Validate pointers
    auto vcamApi = reinterpret_cast<VCamAPI *>(vcam);

    if (!vcamApi || !buffer_size)
        return -EINVAL;

    // Get client executable path
    auto clientPath = vcamApi->m_bridge.clientExe(pid);

    if (clientPath.empty())
        return -EINVAL;

    // Set required buffer size
    *buffer_size = clientPath.size() + 1; // Include null terminator

    // Copy path if buffer is provided
    if (path && *buffer_size) {
        size_t copySize = std::min<size_t>(clientPath.size(), *buffer_size - 1);
        std::copy_n(clientPath.c_str(), copySize, path);
        path[copySize] = '\x0';

        return static_cast<int>(copySize);
    }

    return 0;
}

VCamAPI::~VCamAPI()
{
    if (this->m_eventListener) {
        this->m_bridge.disconnectDevicesChanged(this, &VCamAPI::devicesChanged);
        this->m_bridge.disconnectPictureChanged(this, &VCamAPI::pictureChanged);
        this->m_eventListener = nullptr;
        this->m_context = nullptr;
    }
}

void VCamAPI::devicesChanged(void *context, const std::vector<std::string> &)
{
    auto vcamApi = reinterpret_cast<VCamAPI *>(context);

    if (vcamApi->m_eventListener)
        vcamApi->m_eventListener(vcamApi->m_context, "DevicesUpdated");
}

void VCamAPI::pictureChanged(void *context, const std::string &)
{
    auto vcamApi = reinterpret_cast<VCamAPI *>(context);

    if (vcamApi->m_eventListener)
        vcamApi->m_eventListener(vcamApi->m_context, "PictureUpdated");
}

void VCamAPI::loadGenerals(AkVCam::Settings &settings)
{
    settings.beginGroup("General");

    if (settings.contains("default_frame"))
        this->m_bridge.setPicture(settings.value("default_frame"));

    if (settings.contains("loglevel")) {
        auto logLevel = settings.value("loglevel");
        char *p = nullptr;
        auto level = strtol(logLevel.c_str(), &p, 10);

        if (*p)
            level = AkVCam::Logger::levelFromString(logLevel);

        this->m_bridge.setLogLevel(level);
    }

    settings.endGroup();
}

VCamAPI::StringMatrix VCamAPI::matrixCombine(const StringMatrix &matrix) const
{
    StringVector combined;
    StringMatrix combinations;
    this->matrixCombineP(matrix, 0, combined, combinations);

    return combinations;
}

/* A matrix is a list of lists where each element in the main list is a row,
 * and each element in a row is a column. We combine each element in a row with
 * each element in the next rows.
 */
void VCamAPI::matrixCombineP(const StringMatrix &matrix,
                             size_t index,
                             StringVector combined,
                             StringMatrix &combinations) const
{
    if (index >= matrix.size()) {
        combinations.push_back(combined);

        return;
    }

    for (auto &data: matrix[index]) {
        auto combinedP1 = combined;
        combinedP1.push_back(data);
        this->matrixCombineP(matrix, index + 1, combinedP1, combinations);
    }
}

std::vector<AkVCam::VideoFormat> VCamAPI::readFormat(AkVCam::Settings &settings) const
{
    std::vector<AkVCam::VideoFormat> formats;

    auto pixFormats = settings.valueList("format", ",");
    auto widths = settings.valueList("width", ",");
    auto heights = settings.valueList("height", ",");
    auto frameRates = settings.valueList("fps", ",");

    if (pixFormats.empty()
        || widths.empty()
        || heights.empty()
        || frameRates.empty()) {
        return {};
    }

    StringMatrix formatMatrix;
    formatMatrix.push_back(pixFormats);
    formatMatrix.push_back(widths);
    formatMatrix.push_back(heights);
    formatMatrix.push_back(frameRates);

    for (auto &formatList: this->matrixCombine(formatMatrix)) {
        auto pixFormat = AkVCam::pixelFormatFromCommonString(formatList[0]);
        char *p = nullptr;
        auto width = strtol(formatList[1].c_str(), &p, 10);
        p = nullptr;
        auto height = strtol(formatList[2].c_str(), &p, 10);
        AkVCam::Fraction frameRate(formatList[3]);
        AkVCam::VideoFormat format(pixFormat,
                                   width,
                                   height,
                                   {frameRate});

        if (format.isValid())
            formats.push_back(format);
    }

    return formats;
}

VCamAPI::VideoFormatMatrix VCamAPI::readFormats(AkVCam::Settings &settings) const
{
    VideoFormatMatrix formatsMatrix;
    settings.beginGroup("Formats");
    auto nFormats = settings.beginArray("formats");

    for (size_t i = 0; i < nFormats; i++) {
        settings.setArrayIndex(i);
        formatsMatrix.push_back(this->readFormat(settings));
    }

    settings.endArray();
    settings.endGroup();

    return formatsMatrix;
}

std::vector<AkVCam::VideoFormat> VCamAPI::readDeviceFormats(AkVCam::Settings &settings,
                                                            const VideoFormatMatrix &availableFormats) const
{
    std::vector<AkVCam::VideoFormat> formats;
    auto formatsIndex = settings.valueList("formats", ",");

    for (auto &indexStr: formatsIndex) {
        char *p = nullptr;
        auto index = strtoul(indexStr.c_str(), &p, 10);

        if (*p)
            continue;

        index--;

        if (index >= availableFormats.size())
            continue;

        for (auto &format: availableFormats[index])
            formats.push_back(format);
    }

    return formats;
}

void VCamAPI::createDevice(AkVCam::Settings &settings, const VideoFormatMatrix &availableFormats)
{
    auto description = settings.value("description");

    if (description.empty())
        return;

    auto formats = this->readDeviceFormats(settings, availableFormats);

    if (formats.empty())
        return;

    auto deviceId = settings.value("id");
    deviceId = this->m_bridge.addDevice(description, deviceId);
    auto supportedFormats =
            this->m_bridge.supportedPixelFormats(AkVCam::IpcBridge::StreamType_Output);

    for (auto &format: formats) {
        auto it = std::find(supportedFormats.begin(),
                            supportedFormats.end(),
                            format.format());

        if (it != supportedFormats.end())
            this->m_bridge.addFormat(deviceId, format, -1);
    }
}

void VCamAPI::createDevices(AkVCam::Settings &settings, const VideoFormatMatrix &availableFormats)
{
    for (auto &device: this->m_bridge.devices())
        this->m_bridge.removeDevice(device);

    settings.beginGroup("Cameras");
    size_t nCameras = settings.beginArray("cameras");

    for (size_t i = 0; i < nCameras; i++) {
        settings.setArrayIndex(i);
        this->createDevice(settings, availableFormats);
    }

    settings.endArray();
    settings.endGroup();
    this->m_bridge.updateDevices();
}
