#pragma once

#include <algorithm>
#include <string>
#include <string_view>

namespace utils {
inline std::string toLower(std::string_view str) {
    std::string result{str};
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}
} // namespace utils
