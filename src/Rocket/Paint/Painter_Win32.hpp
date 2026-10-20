/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <vector>
#include <cstdint>

/* <d3d12.h> stays out of this header, as <windows.h> does (see App_Win32.hpp):
   textures are handed around as opaque pointers. */

namespace Rocket {

void* __CreateTexture(std::uint32_t width, std::uint32_t height);

/**
 * Releases a texture once the GPU, and every command list still being
 * recorded, is done with it.
 */
void __ReleaseTexture(void* texture);

/**
 * Uploads tightly packed premultiplied RGBA8 pixels into a region of a
 * texture. The upload runs before any paint pass submitted after it.
 */
void __UploadTexture(void* texture, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, std::uint8_t const* pixels);

void __DownloadTexture(void* texture, std::vector<std::uint8_t>& pixels);

void __CopyTexture(void* source, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, void* destination, std::uint32_t toX, std::uint32_t toY);

void __ReleaseWindowTarget(void* handle);

} /* namespace Rocket */
