/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef AKVCAM_SETTINGS_H
#define AKVCAM_SETTINGS_H

#include <cstdint>
#include <string>
#include <vector>

namespace AkVCam
{
    class SettingsPrivate;
    class Fraction;

    class Settings
    {
        public:
            Settings();
            ~Settings();

            bool load(const std::string &fileName);
            void beginGroup(const std::string &prefix);
            void endGroup();
            size_t beginArray(const std::string &prefix);
            void setArrayIndex(size_t i);
            void endArray();
            std::vector<std::string> groups() const;
            std::vector<std::string> keys() const;
            void clear();
            bool contains(const std::string &key) const;
            std::string value(const std::string &key) const;
            bool valueBool(const std::string &key) const;
            int32_t valueInt32(const std::string &key) const;
            uint32_t valueUInt32(const std::string &key) const;
            std::vector<std::string> valueList(const std::string &key,
                                               const std::string &separators) const;
            Fraction valueFrac(const std::string &key) const;

        private:
            SettingsPrivate *d;
    };
}

#endif // AKVCAM_SETTINGS_H
