/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <vector>
#include <cstring>
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Image.hpp>

namespace Rocket {

/** Rescales tightly-packed RGBA8 pixels on the CPU (nearest or bilinear). */
static void _ScalePixels(std::vector<std::uint8_t> const& src, std::uint32_t srcWidth, std::uint32_t srcHeight, std::vector<std::uint8_t>& dst, std::uint32_t dstWidth, std::uint32_t dstHeight, bool linear) {
    PROFILE

    dst.resize((std::size_t)dstWidth * dstHeight * 4);

    for (std::uint32_t y = 0; y < dstHeight; y++) {
        for (std::uint32_t x = 0; x < dstWidth; x++) {
            auto const out = ((((std::size_t)y * dstWidth) + x) * 4);

            if (linear) {
                auto const fx = ((((float)x + 0.5f) * srcWidth / dstWidth) - 0.5f);
                auto const fy = ((((float)y + 0.5f) * srcHeight / dstHeight) - 0.5f);
                auto const x0 = std::clamp((std::int32_t)std::floor(fx), 0, (std::int32_t)srcWidth - 1);
                auto const y0 = std::clamp((std::int32_t)std::floor(fy), 0, (std::int32_t)srcHeight - 1);
                auto const x1 = std::min(x0 + 1, (std::int32_t)srcWidth - 1);
                auto const y1 = std::min(y0 + 1, (std::int32_t)srcHeight - 1);
                auto const tx = std::clamp(fx - (float)x0, 0.0f, 1.0f);
                auto const ty = std::clamp(fy - (float)y0, 0.0f, 1.0f);

                for (auto c = 0; c < 4; c++) {
                    auto const p00 = (float)src[((((std::size_t)y0 * srcWidth) + x0) * 4) + c];
                    auto const p10 = (float)src[((((std::size_t)y0 * srcWidth) + x1) * 4) + c];
                    auto const p01 = (float)src[((((std::size_t)y1 * srcWidth) + x0) * 4) + c];
                    auto const p11 = (float)src[((((std::size_t)y1 * srcWidth) + x1) * 4) + c];
                    auto const top = (p00 + ((p10 - p00) * tx));
                    auto const bottom = (p01 + ((p11 - p01) * tx));
                    dst[out + c] = (std::uint8_t)std::clamp((int)std::lround(top + ((bottom - top) * ty)), 0, 255);
                }
            } else {
                auto const sx = std::min((std::uint32_t)(((std::uint64_t)x * srcWidth) / dstWidth), srcWidth - 1);
                auto const sy = std::min((std::uint32_t)(((std::uint64_t)y * srcHeight) / dstHeight), srcHeight - 1);
                auto const in = ((((std::size_t)sy * srcWidth) + sx) * 4);

                std::memcpy(&dst[out], &src[in], 4);
            }
        }
    }
}

Image::~Image() {
    PROFILE

    __done();
}

Image::Image()
    : _impl(nullptr)
    , _size({ 1.0f, 1.0f })
{
    PROFILE

    __init(_size);
}

Image::Image(Vec2 const& size)
    : _impl(nullptr)
    , _size(size)
{
    PROFILE

    __init(size);
}

Image::Image(Vec2 const& size, std::uint8_t const* data)
    : _impl(nullptr)
    , _size(size)
{
    PROFILE

    __init(size);

    setData(size, data, ImageDataFormat::RGBA, { 0.0f, 0.0f });
}

Image::Image(Vec2 const& size, std::uint8_t const* data, ImageDataFormat format)
    : _impl(nullptr)
    , _size(size)
{
    PROFILE

    __init(size);

    setData(size, data, format, { 0.0f, 0.0f });
}

Image::Image(std::string const& path)
    : _impl(nullptr)
    , _size({ 1.0f, 1.0f })
{
    PROFILE

    __init(path);
}

Vec2 const& Image::getSize() const {
    PROFILE

    return _size;
}

void* Image::getTexture() const {
    PROFILE

    assert(_impl != nullptr);

    return __getTexture();
}

void Image::setSize(Vec2 const& newSize, ImageFilter filter) {
    PROFILE

    assert(_impl != nullptr);

    auto const oldWidth = std::max(1u, (std::uint32_t)_size.width);
    auto const oldHeight = std::max(1u, (std::uint32_t)_size.height);
    auto const newWidth = std::max(1u, (std::uint32_t)newSize.width);
    auto const newHeight = std::max(1u, (std::uint32_t)newSize.height);

    auto oldPixels = std::vector<std::uint8_t>();
    __getData(oldPixels);

    auto newPixels = std::vector<std::uint8_t>();
    _ScalePixels(oldPixels, oldWidth, oldHeight, newPixels, newWidth, newHeight, (filter == ImageFilter::Linear));

    __resize(newSize);
    __setData({ (float)newWidth, (float)newHeight }, newPixels.data(), { 0.0f, 0.0f });

    _size = newSize;
}

void Image::setData(Vec2 const& size, std::uint8_t const* data, ImageDataFormat format) {
    PROFILE

    setData(size, data, format, { 0.0f, 0.0f });
}

void Image::setData(Vec2 const& size, std::uint8_t const* data, ImageDataFormat format, Vec2 const& offset) {
    PROFILE

    static thread_local auto _buffer = std::vector<std::uint8_t>();

    assert(_impl != nullptr);

    if (data == nullptr) {
        throw std::runtime_error("`data` is required");
    }

    auto pSize = (int)(size.width * size.height);
    auto bSize = pSize * 4;

    _buffer.resize(bSize);

    if (format == ImageDataFormat::RGBA) {
        std::memcpy(_buffer.data(), data, bSize);
    } else if (format == ImageDataFormat::RGB) {
        for (int i = 0; i < pSize; i++) {
            _buffer[(i * 4) + 0] = data[(i * 3) + 0];
            _buffer[(i * 4) + 1] = data[(i * 3) + 1];
            _buffer[(i * 4) + 2] = data[(i * 3) + 2];
            _buffer[(i * 4) + 3] = 255;
        }
    } else if (format == ImageDataFormat::BGRA) {
        for (int i = 0; i < pSize; i++) {
            _buffer[(i * 4) + 0] = data[(i * 4) + 2];
            _buffer[(i * 4) + 1] = data[(i * 4) + 1];
            _buffer[(i * 4) + 2] = data[(i * 4) + 0];
            _buffer[(i * 4) + 3] = data[(i * 4) + 3];
        }
    } else if (format == ImageDataFormat::BGR) {
        for (int i = 0; i < pSize; i++) {
            _buffer[(i * 4) + 0] = data[(i * 3) + 2];
            _buffer[(i * 4) + 1] = data[(i * 3) + 1];
            _buffer[(i * 4) + 2] = data[(i * 3) + 0];
            _buffer[(i * 4) + 3] = 255;
        }
    } else if (format == ImageDataFormat::Grayscale) {
        for (int i = 0; i < pSize; i++) {
            _buffer[(i * 4) + 0] = 0;
            _buffer[(i * 4) + 1] = 0;
            _buffer[(i * 4) + 2] = 0;
            _buffer[(i * 4) + 3] = data[i];
        }
    }

    for (int i = 0; i < pSize; i++) {
        auto const a = (int)_buffer[(i * 4) + 3];
        _buffer[(i * 4) + 0] = (std::uint8_t)(((int)_buffer[(i * 4) + 0] * a + 127) / 255);
        _buffer[(i * 4) + 1] = (std::uint8_t)(((int)_buffer[(i * 4) + 1] * a + 127) / 255);
        _buffer[(i * 4) + 2] = (std::uint8_t)(((int)_buffer[(i * 4) + 2] * a + 127) / 255);
    }

    __setData(size, _buffer.data(), offset);
}

void Image::getData(std::vector<std::uint8_t>& data) const {
    PROFILE

    assert(_impl != nullptr);

    data.clear();

    __getData(data);
}

void Image::copyFrom(Image const& image, Vec4 const& slice, Vec2 const& position) {
    PROFILE

    assert(_impl != nullptr);

    __copyFrom(image, slice, position);
}

} /* namespace Rocket */
