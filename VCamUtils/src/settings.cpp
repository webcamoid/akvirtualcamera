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

#include <algorithm>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

#include "settings.h"
#include "fraction.h"
#include "logger.h"

namespace AkVCam
{
    using GroupMap = std::map<std::string, std::string>;

    struct SettingsElement
    {
        std::string group;
        std::string key;
        std::string value;

        inline bool empty();
    };

    class SettingsPrivate
    {
        public:
            std::map<std::string, GroupMap> m_configs;
            std::string m_currentGroup;
            std::string m_currentArray;
            size_t m_arrayIndex;

            SettingsElement parse(const std::string &line, bool *ok);
            std::string parseString(const std::string &str);
            GroupMap groupConfigs();
    };
}

AkVCam::Settings::Settings()
{
    this->d = new SettingsPrivate;
}

AkVCam::Settings::~Settings()
{
    this->endArray();
    this->endGroup();
    delete this->d;
}

bool AkVCam::Settings::load(const std::string &fileName)
{
    this->clear();

    if (fileName.empty()) {
        AkLogError() << "Settings file name not valid" << std::endl;

        return false;
    }

    std::ifstream configFile(fileName);

    if (!configFile.is_open()) {
        AkLogError() << "Can't open settings file: " << fileName << std::endl;

        return false;
    }

    std::string currentGroup;

    while (!configFile.eof()) {
        std::string line;
        std::getline(configFile, line);

        bool ok = false;
        auto element = this->d->parse(line, &ok);

        if (!ok) {
            AkLogError() << "Error parsing settings file: " << fileName << std::endl;
            AkLogError() << "Line: " << line << std::endl;
            this->clear();
            configFile.close();

            return false;
        }

        if (!element.empty()) {
            if (!element.group.empty()) {
                currentGroup = element.group;

                if (this->d->m_configs.count(currentGroup) < 1)
                    this->d->m_configs[currentGroup] = {};
            } else if (!element.key.empty() && !element.value.empty()) {
                if (currentGroup.empty()) {
                    currentGroup = "General";
                    this->d->m_configs[currentGroup] = {};
                }

                this->d->m_configs[currentGroup][element.key] = element.value;
            }
        }
    }

    configFile.close();

    return true;
}

void AkVCam::Settings::beginGroup(const std::string &prefix)
{
    this->endGroup();
    this->d->m_currentGroup = prefix;
}

void AkVCam::Settings::endGroup()
{
    this->d->m_currentGroup = {};
}

size_t AkVCam::Settings::beginArray(const std::string &prefix)
{
    auto groupConfigs = this->d->groupConfigs();

    if (groupConfigs.empty())
        return 0;

    this->endArray();

    // Read array size.
    this->d->m_currentArray = prefix;
    auto arraySizeStr = groupConfigs[prefix + "/size"];
    char *p = nullptr;

    return strtoul(arraySizeStr.c_str(), &p, 10);
}

void AkVCam::Settings::setArrayIndex(size_t i)
{
    this->d->m_arrayIndex = i;
}

void AkVCam::Settings::endArray()
{
    this->d->m_currentArray = {};
}

std::vector<std::string> AkVCam::Settings::groups() const
{
    std::vector<std::string> groups;

    for (auto it = this->d->m_configs.begin();
         it != this->d->m_configs.end();
         it++) {
        groups.push_back(it->first);
    }

    return groups;
}

std::vector<std::string> AkVCam::Settings::keys() const
{
    std::vector<std::string> keys;
    auto groupConfigs = this->d->groupConfigs();

    for (auto it = groupConfigs.begin();
         it != groupConfigs.end();
         it++) {
        keys.push_back(it->first);
    }

    return keys;
}

void AkVCam::Settings::clear()
{
    this->d->m_configs.clear();
    this->endArray();
    this->endGroup();
    this->d->m_arrayIndex = 0;
}

bool AkVCam::Settings::contains(const std::string &key) const
{
    if (key.empty())
        return false;

    auto groupConfigs = this->d->groupConfigs();
    bool contains;

    if (groupConfigs.empty())
        return false;

    if (this->d->m_currentArray.empty()) {
        contains = groupConfigs.count(key) > 0;
    } else {
        std::string arrayKey;
        std::stringstream ss(arrayKey);
        ss << this->d->m_currentArray
           << '/'
           << this->d->m_arrayIndex + 1
           << '/'
           << key;
        contains = groupConfigs.count(arrayKey) > 0;
    }

    return contains;
}

std::string AkVCam::Settings::value(const std::string &key) const
{
    if (key.empty())
        return {};

    auto groupConfigs = this->d->groupConfigs();
    std::string value;

    if (groupConfigs.empty())
        return {};

    if (this->d->m_currentArray.empty()) {
        value = groupConfigs[key];
    } else {
        std::string arrayKey;
        std::stringstream ss(arrayKey);
        ss << this->d->m_currentArray
           << '/'
           << this->d->m_arrayIndex + 1
           << '/'
           << key;
        value = groupConfigs[arrayKey];
    }

    return value;
}

bool AkVCam::Settings::valueBool(const std::string &key) const
{
    auto value = this->value(key);

    if (value.empty())
        return false;

    if (value == "true")
        return true;

    char *p = nullptr;

    return strtol(value.c_str(), &p, 10) != 0;
}

int32_t AkVCam::Settings::valueInt32(const std::string &key) const
{
    auto value = this->value(key);

    if (value.empty())
        return 0;

    char *p = nullptr;

    return strtol(value.c_str(), &p, 10);
}

uint32_t AkVCam::Settings::valueUInt32(const std::string &key) const
{
    auto value = this->value(key);

    if (value.empty())
        return 0;

    char *p = nullptr;

    return strtoul(value.c_str(), &p, 10);
}

std::vector<std::string> AkVCam::Settings::valueList(const std::string &key,
                                                     const std::string &separators) const
{
    auto value = this->value(key);

    if (value.empty())
        return {};

    std::vector<std::string> result;
    size_t pos = 0;

   do {
        auto index = value.size();

        for (auto &separator: separators) {
            auto it = std::find(value.begin() + pos, value.end(), separator);

            if (size_t(it - value.begin()) < index)
                index = it - value.begin();
        }

        result.push_back(trimmed(value.substr(pos, index - pos)));
        pos = index + 1;
    } while (pos < value.size());

    return result;
}

AkVCam::Fraction AkVCam::Settings::valueFrac(const std::string &key) const
{
    auto fracList = this->valueList(key, "/");

    if (fracList.empty())
        return {};

    Fraction frac(0, 1);
    char *p = nullptr;

    switch (fracList.size()) {
    case 1:
        frac.num() = strtoul(fracList[0].c_str(), &p, 10);

        break;

    case 2:
        frac.num() = strtoul(fracList[0].c_str(), &p, 10);
        p = nullptr;
        frac.den() = strtoul(fracList[1].c_str(), &p, 10);

        if (frac.den() < 1) {
            frac.num() = 0;
            frac.den() = 1;
        }

        break;

    default:
        break;
    }

    return frac;
}

AkVCam::SettingsElement AkVCam::SettingsPrivate::parse(const std::string &line, bool *ok)
{
    SettingsElement element;

    if (line.empty() || line[0] == '#' || line[0] == ';') {
        if (ok)
            *ok = false;

        return {};
    }

    if (line[0] == '[') {
        if (line[line.size() - 1] != ']' || line.size() < 3) {
            if (ok)
                *ok = false;

            return {};
        }

        element.group = trimmed(line.substr(1, line.size() - 2));

        if (ok)
            *ok = true;

        return element;
    }

    if (line.find('=') == std::string::npos) {
        if (ok)
            *ok = false;

        return {};
    }

    auto pair = splitOnce(line, "=");
    element.key = trimmed(pair.first);
    std::replace(element.key.begin(), element.key.end(), '\\', '/');

    if (element.key.empty()) {
        if (ok)
            *ok = false;

        return {};
    }

    element.value = trimmed(pair.second);
    element.value = this->parseString(element.value);

    if (ok)
        *ok = true;

    return element;
}

/* Escape sequences taken from:
 *
 * https://en.cppreference.com/w/cpp/language/escape
 *
 * but we ignore octals and universal characters.
 */
std::string AkVCam::SettingsPrivate::parseString(const std::string &str)
{
    if (str.size() < 2)
        return str;

    const char escape_k[] = "'\"?\\abfnrtv0";
    const char escape_v[] = "'\"?\\\a\b\f\n\r\t\v\0";

    auto c = str[0];
    size_t start;
    size_t end;

    if ((c == '"' || c == '\'') && str[str.size() - 1] == c) {
        start = 1;
        end  = str.size() - 1;
    } else {
        start = 0;
        end  = str.size();
    }

    std::string parsedStr;
    char hex[3];
    memset(hex, 0, 3);

    for (size_t i = start; i < end; i++) {
        if (str[i] == '\\' && i < str.size() - 2) {
            auto key = strchr(escape_k, str[i + 1]);

            if (key) {
                parsedStr += escape_v[key - escape_k];
                i++;
            } else if (str[i + 1] == 'x' && i < str.size() - 4) {
                memcpy(hex, str.c_str() + i + 2, 2);
                char *p = nullptr;
                c = strtol(hex, &p, 16);

                if (*p) {
                    parsedStr += str[i];
                } else {
                    parsedStr += c;
                    i += 3;
                }
            } else {
                parsedStr += str[i];
            }
        } else {
            parsedStr += str[i];
        }
    }

    return parsedStr;
}

AkVCam::GroupMap AkVCam::SettingsPrivate::groupConfigs()
{
    GroupMap groupConfigs;

    if (this->m_currentGroup.empty())
        groupConfigs = this->m_configs["General"];
    else
        groupConfigs = this->m_configs[this->m_currentGroup];

    return groupConfigs;
}

bool AkVCam::SettingsElement::empty()
{
    return this->group.empty()
            && this->key.empty()
            && this->value.empty();
}
