/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <vector>
#include <cstdint>

/* The GL headers stay out of this header: textures are handed around as
   opaque pointers, owned by Painter_GLES.cpp. */

namespace Rocket {

void* __CreateTexture(std::uint32_t width, std::uint32_t height);

/**
 * Releases a texture once every pass still being recorded, which may draw
 * with it or into it, has run.
 */
void __ReleaseTexture(void* texture);

/**
 * Uploads tightly packed premultiplied RGBA8 pixels into a region of a
 * texture, row 0 at the top. The upload runs before any paint pass still
 * being recorded.
 */
void __UploadTexture(void* texture, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, std::uint8_t const* pixels);

/** Downloads a texture as tightly packed premultiplied RGBA8, row 0 at the top. */
void __DownloadTexture(void* texture, std::vector<std::uint8_t>& pixels);

void __CopyTexture(void* source, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, void* destination, std::uint32_t toX, std::uint32_t toY);

} /* namespace Rocket */
