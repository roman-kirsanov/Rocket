/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <vector>
#include <cstdint>

namespace Rocket {

void* __CreateTexture(std::uint32_t width, std::uint32_t height);
void __ReleaseTexture(void* texture);
void __DownloadTexture(void* texture, std::vector<std::uint8_t>& pixels);
void __UploadTexture(void* texture, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, std::uint8_t const* pixels);
void __CopyTexture(void* source, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, void* destination, std::uint32_t toX, std::uint32_t toY);

} /* namespace Rocket */
