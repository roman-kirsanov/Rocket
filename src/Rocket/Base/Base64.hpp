/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Rocket {

/**
 * Encodes bytes as standard base64 (RFC 4648), padded with '='.
 *
 * @param bytes The bytes to encode.
 * @return The base64 text; empty for no bytes.
 */
std::string EncodeBase64(std::vector<std::uint8_t> const& bytes);

/**
 * Decodes standard base64 text into bytes.
 *
 * Padding is optional: decoding stops at the first '='.
 *
 * @param text The base64 text.
 * @return The decoded bytes.
 * @throws std::runtime_error if the text holds a character that is not base64.
 */
std::vector<std::uint8_t> DecodeBase64(std::string const& text);

} /* namespace Rocket */
