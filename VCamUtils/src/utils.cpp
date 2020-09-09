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

#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>

#include "utils.h"

uint64_t AkVCam::id()
{
    static uint64_t id = 0;

    return id++;
}

std::string AkVCam::timeStamp()
{
    char ts[256];
    auto time = std::time(nullptr);
    strftime(ts, 256, "%Y%m%d%H%M%S", std::localtime(&time));

    return std::string(ts);
}

std::string AkVCam::replace(const std::string &str,
                            const std::string &from,
                            const std::string &to)
{
    auto newStr = str;

    for (auto pos = newStr.find(from);
         pos != std::string::npos;
         pos = newStr.find(from))
        newStr.replace(pos, from.size(), to);

    return newStr;
}

std::wstring AkVCam::replace(const std::wstring &str,
                             const std::wstring &from,
                             const std::wstring &to)
{
    auto newStr = str;

    for (auto pos = newStr.find(from);
         pos != std::wstring::npos;
         pos = newStr.find(from))
        newStr.replace(pos, from.size(), to);

    return newStr;
}

bool AkVCam::isEqualFile(const std::wstring &file1, const std::wstring &file2)
{
    if (file1 == file2)
        return true;

    std::fstream f1;
    std::fstream f2;
    f1.open(std::string(file1.begin(), file1.end()), std::ios_base::in);
    f2.open(std::string(file2.begin(), file2.end()), std::ios_base::in);

    if (!f1.is_open() || !f2.is_open())
        return false;

    const size_t bufferSize = 1024;
    char buffer1[bufferSize];
    char buffer2[bufferSize];
    memset(buffer1, 0, bufferSize);
    memset(buffer2, 0, bufferSize);

    while (!f1.eof() && !f2.eof()) {
        f1.read(buffer1, bufferSize);
        f2.read(buffer2, bufferSize);

        if (memcmp(buffer1, buffer2, bufferSize) != 0)
            return false;
    }

    return true;
}

std::string AkVCam::trimmed(const std::string &str)
{
    auto left = uint64_t(str.size());
    auto right = uint64_t(str.size());

    for (size_t i = 0; i < str.size(); i++)
        if (!isspace(str[i])) {
            left = uint64_t(i);

            break;
        }

    auto strippedLen = str.size();

    if (left == str.size()) {
        strippedLen = 0;
    } else {
        for (int64_t i = str.size() - 1; i >= 0; i--)
            if (!isspace(str[size_t(i)])) {
                right = uint64_t(i);

                break;
            }

        strippedLen = size_t(right - left + 1);
    }

    return str.substr(size_t(left), strippedLen);
}

std::wstring AkVCam::trimmed(const std::wstring &str)
{
    auto left = uint64_t(str.size());
    auto right = uint64_t(str.size());

    for (size_t i = 0; i < str.size(); i++)
        if (!iswspace(str[i])) {
            left = uint64_t(i);

            break;
        }

    auto strippedLen = str.size();

    if (left == str.size()) {
        strippedLen = 0;
    } else {
        for (int64_t i = str.size() - 1; i >= 0; i--)
            if (!iswspace(str[size_t(i)])) {
                right = uint64_t(i);

                break;
            }

        strippedLen = size_t(right - left + 1);
    }

    return str.substr(size_t(left), strippedLen);
}

std::string AkVCam::fill(const std::string &str, size_t maxSize)
{
    std::stringstream ss;
    std::vector<char> spaces(maxSize, ' ');
    ss << str << std::string(spaces.data(), maxSize - str.size());

    return ss.str();
}

std::wstring AkVCam::fill(const std::wstring &str, size_t maxSize)
{
    std::wstringstream ss;
    std::vector<wchar_t> spaces(maxSize, ' ');
    ss << str << std::wstring(spaces.data(), maxSize - str.size());

    return ss.str();
}

std::string AkVCam::join(const std::vector<std::string> &strs,
                         const std::string &separator)
{
    std::stringstream ss;

    for (size_t i = 0; i < strs.size(); i++) {
        if (i > 0)
            ss << separator;

        ss << strs[i];
    }

    return ss.str();
}

std::vector<std::string> AkVCam::split(const std::string &str, char separator)
{
    if (str.empty())
        return {};

    std::vector<std::string> elements;
    std::string subStr;

    for (auto &c: str)
        if (c == separator) {
            elements.push_back(subStr);
            subStr.clear();
        } else {
            subStr += c;
        }

    if (!subStr.empty() || *str.rbegin() == separator)
        elements.push_back(subStr);

    return elements;
}

std::pair<std::string, std::string> AkVCam::splitOnce(const std::string &str,
                                                      const std::string &separator)
{
    auto pos = str.find(separator);

    if (pos == std::string::npos)
        return {str, ""};

    auto first = str.substr(0, pos);
    auto second = pos + 1 < str.size()? str.substr(pos + 1): "";

    return {first, second};
}
