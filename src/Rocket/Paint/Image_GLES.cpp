/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Painter_GLES.hpp>

namespace Rocket {

struct Image::_Image {
    void* texture = nullptr;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

void Image::__init(Vec2 const& size) {
    PROFILE

    _impl = new _Image{};
    _impl->width = std::max(1u, (std::uint32_t)size.width);
    _impl->height = std::max(1u, (std::uint32_t)size.height);
    _impl->texture = __CreateTexture(_impl->width, _impl->height);
}

void Image::__init(std::string const& path) {
    PROFILE

    auto loaded = ::IMG_Load(path.c_str());

    if (loaded == nullptr) {
        throw std::runtime_error("Failed to open image: " + path);
    }

    auto surface = ::SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    ::SDL_DestroySurface(loaded);

    if (surface == nullptr) {
        throw std::runtime_error("Failed to convert image: " + path);
    }

    auto const width = (std::uint32_t)surface->w;
    auto const height = (std::uint32_t)surface->h;

    // Straight RGBA rows, packed tight; setData premultiplies them the way it
    // does every other image's pixels.
    auto pixels = std::vector<std::uint8_t>((std::size_t)width * height * 4);

    for (std::uint32_t row = 0; row < height; row++) {
        std::memcpy(
            pixels.data() + ((std::size_t)row * width * 4),
            static_cast<std::uint8_t const*>(surface->pixels) + ((std::size_t)row * surface->pitch),
            ((std::size_t)width * 4)
        );
    }

    ::SDL_DestroySurface(surface);

    __init(Vec2{ (float)width, (float)height });
    _size = { (float)width, (float)height };

    setData(_size, pixels.data(), ImageDataFormat::RGBA);
}

void Image::__done() {
    PROFILE

    if (_impl != nullptr) {
        __ReleaseTexture(_impl->texture);
        delete _impl;
        _impl = nullptr;
    }
}

void* Image::__getTexture() const {
    PROFILE

    return _impl->texture;
}

void Image::__resize(Vec2 const& size) {
    PROFILE

    __ReleaseTexture(_impl->texture);

    _impl->width = std::max(1u, (std::uint32_t)size.width);
    _impl->height = std::max(1u, (std::uint32_t)size.height);
    _impl->texture = __CreateTexture(_impl->width, _impl->height);
}

void Image::__setData(Vec2 const& size, std::uint8_t const* pixels, Vec2 const& offset) {
    PROFILE

    __UploadTexture(
        _impl->texture,
        (std::uint32_t)offset.x,
        (std::uint32_t)offset.y,
        (std::uint32_t)size.width,
        (std::uint32_t)size.height,
        pixels
    );
}

void Image::__getData(std::vector<std::uint8_t>& data) const {
    PROFILE

    __DownloadTexture(_impl->texture, data);
}

void Image::__copyFrom(Image const& image, Vec4 const& slice, Vec2 const& position) {
    PROFILE

    __CopyTexture(
        image._impl->texture,
        (std::uint32_t)slice.x,
        (std::uint32_t)slice.y,
        (std::uint32_t)slice.width,
        (std::uint32_t)slice.height,
        _impl->texture,
        (std::uint32_t)position.x,
        (std::uint32_t)position.y
    );
}

} /* namespace Rocket */
