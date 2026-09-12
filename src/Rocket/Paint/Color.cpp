/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/Paint/Color.hpp>

namespace Rocket {

std::string ColorToHex(Vec4 const& color) {
    PROFILE

    auto ri = static_cast<int>(color.red * 255.0f);
    auto gi = static_cast<int>(color.green * 255.0f);
    auto bi = static_cast<int>(color.blue * 255.0f);
    auto ai = static_cast<int>(color.alpha * 255.0f);

    auto oss = std::ostringstream();

    oss << "#" << std::hex << std::setw(2) << std::setfill('0') << ri
        << std::setw(2) << std::setfill('0') << gi
        << std::setw(2) << std::setfill('0') << bi
        << std::setw(2) << std::setfill('0') << ai;

    return oss.str();
}

Vec4 ColorFromHex(std::string const& hex) {
    PROFILE

    if (hex.empty() || hex[0] != '#') {
        return {};
    }

    auto len = hex.size();
    if (len != 4 && len != 5 && len != 7 && len != 9) {
        return {};
    }

    auto nhex = std::string();
    if (len == 4 || len == 5) {
        for (std::size_t i = 1; i < len; i++) {
            nhex += hex[i];
            nhex += hex[i];
        }
    } else {
        nhex = hex.substr(1);
    }

    if (nhex.size() == 6) {
        nhex += "ff";
    }

    auto rgba = std::uint32_t{};
    auto ss = std::stringstream();
    ss << std::hex << nhex;
    ss >> rgba;

    return Vec4{
        ((rgba >> 24) & 0xFF) / 255.0f,
        ((rgba >> 16) & 0xFF) / 255.0f,
        ((rgba >> 8) & 0xFF) / 255.0f,
        (rgba & 0xFF) / 255.0f
    };
}

Vec4 ColorFromRGB(std::int32_t const& red, std::int32_t const& green, std::int32_t const& blue) {
    PROFILE

    return Vec4{
        std::clamp(red, 0, 255) / 255.0f,
        std::clamp(green, 0, 255) / 255.0f,
        std::clamp(blue, 0, 255) / 255.0f,
        1.0f
    };
}

Vec4 ColorFromRGBA(std::int32_t const& red, std::int32_t const& green, std::int32_t const& blue, float const& alpha) {
    PROFILE

    return Vec4{
        std::clamp(red, 0, 255) / 255.0f,
        std::clamp(green, 0, 255) / 255.0f,
        std::clamp(blue, 0, 255) / 255.0f,
        alpha
    };
}

} /* namespace Rocket */
