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
    std::string newStr;

    for (size_t i = 0; i < str.size(); i++) {
        auto pos = str.find(from, i);

        if (pos == std::string::npos) {
            newStr += str.substr(i);

            break;
        } else {
            newStr += str.substr(i, pos - i) + to;
            i = pos + from.size() - 1;
        }
    }

    return newStr;
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
        for (auto i = int64_t(str.size() - 1); i >= 0; i--)
            if (!isspace(str[size_t(i)])) {
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

void AkVCam::move(const std::string &from, const std::string &to)
{
    std::ifstream infile(from, std::ios::in | std::ios::binary);
    std::ofstream outfile(to, std::ios::out | std::ios::binary);
    outfile << infile.rdbuf();
    std::remove(from.c_str());
}
