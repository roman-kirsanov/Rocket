/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <vector>
#include <cstring>
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Painter.hpp>
#include <Rocket/Paint/Painter_Private.hpp>

namespace Rocket {

struct Image::_Image {
    SDL_GPUTexture* texture = nullptr;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

static SDL_GPUDevice* _Device() {
    PROFILE

    return static_cast<SDL_GPUDevice*>(__GetDefaultGPUDevice());
}

static SDL_GPUTexture* _CreateTexture(std::uint32_t width, std::uint32_t height) {
    PROFILE

    auto info = SDL_GPUTextureCreateInfo{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = (SDL_GPUTextureFormat)__GetDefaultTextureFormat();
    info.usage = (SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET);
    info.width = std::max(1u, width);
    info.height = std::max(1u, height);
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;

    auto texture = ::SDL_CreateGPUTexture(_Device(), &info);

    if (texture == nullptr) {
        throw std::runtime_error(::SDL_GetError());
    }

    return texture;
}

static SDL_GPUCommandBuffer* _AcquireCommandBuffer() {
    PROFILE

    auto commandBuffer = ::SDL_AcquireGPUCommandBuffer(_Device());

    if (commandBuffer == nullptr) {
        throw std::runtime_error(::SDL_GetError());
    }

    return commandBuffer;
}

static void _UploadPixels(SDL_GPUTexture* texture, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, std::uint8_t const* pixels) {
    PROFILE

    if ((width == 0) || (height == 0)) {
        return;
    }

    auto const dataSize = (std::uint32_t)(width * height * 4);
    auto const bufferInfo = SDL_GPUTransferBufferCreateInfo{ SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, dataSize, 0 };
    auto staging = ::SDL_CreateGPUTransferBuffer(_Device(), &bufferInfo);

    if (staging == nullptr) {
        throw std::runtime_error(::SDL_GetError());
    }

    std::memcpy(::SDL_MapGPUTransferBuffer(_Device(), staging, false), pixels, dataSize);
    ::SDL_UnmapGPUTransferBuffer(_Device(), staging);

    auto const source = SDL_GPUTextureTransferInfo{ staging, 0, width, height };
    auto destination = SDL_GPUTextureRegion{};
    destination.texture = texture;
    destination.x = x;
    destination.y = y;
    destination.w = width;
    destination.h = height;
    destination.d = 1;

    auto commandBuffer = _AcquireCommandBuffer();
    auto copy = ::SDL_BeginGPUCopyPass(commandBuffer);

    ::SDL_UploadToGPUTexture(copy, &source, &destination, false);
    ::SDL_EndGPUCopyPass(copy);
    ::SDL_SubmitGPUCommandBuffer(commandBuffer);
    ::SDL_ReleaseGPUTransferBuffer(_Device(), staging); /* released once the upload is done */
}

static void _DownloadPixels(SDL_GPUTexture* texture, std::uint32_t width, std::uint32_t height, std::vector<std::uint8_t>& pixels) {
    PROFILE

    auto const dataSize = (std::uint32_t)(width * height * 4);
    auto const bufferInfo = SDL_GPUTransferBufferCreateInfo{ SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, dataSize, 0 };
    auto staging = ::SDL_CreateGPUTransferBuffer(_Device(), &bufferInfo);

    if (staging == nullptr) {
        throw std::runtime_error(::SDL_GetError());
    }

    auto source = SDL_GPUTextureRegion{};
    source.texture = texture;
    source.w = width;
    source.h = height;
    source.d = 1;
    auto const destination = SDL_GPUTextureTransferInfo{ staging, 0, width, height };

    auto commandBuffer = _AcquireCommandBuffer();
    auto copy = ::SDL_BeginGPUCopyPass(commandBuffer);

    ::SDL_DownloadFromGPUTexture(copy, &source, &destination);
    ::SDL_EndGPUCopyPass(copy);

    auto fence = ::SDL_SubmitGPUCommandBufferAndAcquireFence(commandBuffer);

    if (fence != nullptr) {
        ::SDL_WaitForGPUFences(_Device(), true, &fence, 1);
        ::SDL_ReleaseGPUFence(_Device(), fence);
    }

    pixels.resize(dataSize);
    std::memcpy(pixels.data(), ::SDL_MapGPUTransferBuffer(_Device(), staging, false), dataSize);
    ::SDL_UnmapGPUTransferBuffer(_Device(), staging);
    ::SDL_ReleaseGPUTransferBuffer(_Device(), staging);
}

void Image::__init(Vec2 const& size) {
    PROFILE

    _impl = new _Image{};
    _impl->texture = _CreateTexture((std::uint32_t)size.width, (std::uint32_t)size.height);
    _impl->width = std::max(1u, (std::uint32_t)size.width);
    _impl->height = std::max(1u, (std::uint32_t)size.height);
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
        ::SDL_ReleaseGPUTexture(_Device(), _impl->texture); /* released once the GPU is done with it */
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

    ::SDL_ReleaseGPUTexture(_Device(), _impl->texture);

    _impl->texture = _CreateTexture((std::uint32_t)size.width, (std::uint32_t)size.height);
    _impl->width = std::max(1u, (std::uint32_t)size.width);
    _impl->height = std::max(1u, (std::uint32_t)size.height);
}

void Image::__setData(Vec2 const& size, std::uint8_t const* pixels, Vec2 const& offset) {
    PROFILE

    _UploadPixels(
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

    _DownloadPixels(_impl->texture, _impl->width, _impl->height, data);
}

void Image::__copyFrom(Image const& image, Vec4 const& slice, Vec2 const& position) {
    PROFILE

    auto const width = (std::uint32_t)slice.width;
    auto const height = (std::uint32_t)slice.height;

    if ((width == 0) || (height == 0)) {
        return;
    }

    auto const source = SDL_GPUTextureLocation{ image._impl->texture, 0, 0, (std::uint32_t)slice.x, (std::uint32_t)slice.y, 0 };
    auto const destination = SDL_GPUTextureLocation{ _impl->texture, 0, 0, (std::uint32_t)position.x, (std::uint32_t)position.y, 0 };

    auto commandBuffer = _AcquireCommandBuffer();
    auto copy = ::SDL_BeginGPUCopyPass(commandBuffer);

    ::SDL_CopyGPUTextureToTexture(copy, &source, &destination, width, height, 1, false);
    ::SDL_EndGPUCopyPass(copy);
    ::SDL_SubmitGPUCommandBuffer(commandBuffer);
}

} /* namespace Rocket */
