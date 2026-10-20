/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <windows.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Painter_Win32.hpp>
#include <Rocket/Window/App_Win32.hpp>

using Microsoft::WRL::ComPtr;

namespace Rocket {

struct Image::_Image {
    void* texture = nullptr;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

static IWICImagingFactory* _GetImagingFactory() {
    PROFILE

    App::__InitWin32();

    static auto const _factory = [] {
        auto factory = ComPtr<IWICImagingFactory>();

        if (FAILED(::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) {
            throw std::runtime_error("Failed to create the WIC imaging factory");
        }

        return factory;
    }();

    return _factory.Get();
}

void Image::__init(Vec2 const& size) {
    PROFILE

    _impl = new _Image{};
    _impl->width = std::max(1u, (std::uint32_t)size.width);
    _impl->height = std::max(1u, (std::uint32_t)size.height);
    _impl->texture = __CreateTexture(_impl->width, _impl->height);
}

void Image::__init(std::string const& path) {
    PROFILE

    auto const factory = _GetImagingFactory();
    auto decoder = ComPtr<IWICBitmapDecoder>();
    auto frame = ComPtr<IWICBitmapFrameDecode>();
    auto converter = ComPtr<IWICFormatConverter>();

    if (FAILED(factory->CreateDecoderFromFilename(App::__ToWide(path).c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder))) {
        throw std::runtime_error("Failed to open image: " + path);
    }

    if (FAILED(decoder->GetFrame(0, &frame))) {
        throw std::runtime_error("Failed to decode image: " + path);
    }

    // Straight RGBA, as the SDL3 backend decodes: setData premultiplies it the
    // way it does every other image's pixels.
    if (
        FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))
    ) {
        throw std::runtime_error("Failed to convert image: " + path);
    }

    auto width = UINT(0);
    auto height = UINT(0);
    converter->GetSize(&width, &height);

    auto pixels = std::vector<std::uint8_t>((std::size_t)width * height * 4);

    if (FAILED(converter->CopyPixels(nullptr, (width * 4), static_cast<UINT>(pixels.size()), pixels.data()))) {
        throw std::runtime_error("Failed to decode image: " + path);
    }

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
