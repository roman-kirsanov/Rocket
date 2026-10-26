/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <algorithm>
#include <format>
#include <stdexcept>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/Base64.hpp>

namespace Rocket {

static auto const _alphabet = std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

std::string EncodeBase64(std::vector<std::uint8_t> const& bytes) {
    PROFILE

    auto text = std::string();

    for (auto index = std::size_t(0); index < bytes.size(); index += 3) {
        auto const count = std::min(std::size_t(3), (bytes.size() - index));
        auto block = (static_cast<std::uint32_t>(bytes[index]) << 16);

        if (count > 1) block |= (static_cast<std::uint32_t>(bytes[index + 1]) << 8);
        if (count > 2) block |= static_cast<std::uint32_t>(bytes[index + 2]);

        text += _alphabet[(block >> 18) & 0x3F];
        text += _alphabet[(block >> 12) & 0x3F];
        text += (count > 1 ? _alphabet[(block >> 6) & 0x3F] : '=');
        text += (count > 2 ? _alphabet[block & 0x3F] : '=');
    }

    return text;
}

std::vector<std::uint8_t> DecodeBase64(std::string const& text) {
    PROFILE

    auto bytes = std::vector<std::uint8_t>();
    auto block = std::uint32_t(0);
    auto bits = 0;

    for (auto const character : text) {
        if (character == '=') {
            break;
        }

        auto const found = _alphabet.find(character);

        if (found == std::string::npos) {
            throw std::runtime_error(std::format("Failed to decode base64: Character is not base64: {}", character));
        }

        block = ((block << 6) | static_cast<std::uint32_t>(found));
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            bytes.push_back(static_cast<std::uint8_t>((block >> bits) & 0xFF));
        }
    }

    return bytes;
}

} /* namespace Rocket */
