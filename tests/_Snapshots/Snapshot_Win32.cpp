/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// The snapshot PNG IO on Windows, through WIC. It lives apart from
// Snapshot.hpp so <windows.h>, and the macros it defines (small, interface,
// min, max), stay out of the tests.

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <windows.h>
#include <objbase.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <Rocket/Paint/Painter_Private.hpp>

using Microsoft::WRL::ComPtr;

namespace Rocket {

bool _IsSoftwareRenderer() {
    auto const device = static_cast<ID3D12Device*>(__GetDefaultGPUDevice());
    auto factory = ComPtr<IDXGIFactory4>();
    auto adapter = ComPtr<IDXGIAdapter1>();

    if (FAILED(::CreateDXGIFactory2(0, IID_PPV_ARGS(&factory))) || FAILED(factory->EnumAdapterByLuid(device->GetAdapterLuid(), IID_PPV_ARGS(&adapter)))) {
        return false;
    }

    auto description = DXGI_ADAPTER_DESC1{};
    adapter->GetDesc1(&description);

    /* WARP, also when it is listed as the Basic Render Driver */
    return ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
        || ((description.VendorId == 0x1414) && (description.DeviceId == 0x8C));
}

static IWICImagingFactory* _GetSnapshotImagingFactory() {
    static auto const _factory = [] {
        ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        auto factory = ComPtr<IWICImagingFactory>();
        ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));

        return factory;
    }();

    return _factory.Get();
}

bool _SaveSnapshotPng(std::string const& path, int width, int height, std::vector<std::uint8_t> const& pixels) {
    auto const factory = _GetSnapshotImagingFactory();
    auto const widePath = std::filesystem::path(path).wstring();
    auto stream = ComPtr<IWICStream>();
    auto encoder = ComPtr<IWICBitmapEncoder>();
    auto frame = ComPtr<IWICBitmapFrameEncode>();
    auto format = GUID_WICPixelFormat32bppRGBA;

    auto const opened =
        (factory != nullptr) &&
        SUCCEEDED(factory->CreateStream(&stream)) &&
        SUCCEEDED(stream->InitializeFromFilename(widePath.c_str(), GENERIC_WRITE)) &&
        SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) &&
        SUCCEEDED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)) &&
        SUCCEEDED(encoder->CreateNewFrame(&frame, nullptr)) &&
        SUCCEEDED(frame->Initialize(nullptr)) &&
        SUCCEEDED(frame->SetSize((UINT)width, (UINT)height)) &&
        SUCCEEDED(frame->SetPixelFormat(&format));

    /* straight RGBA, which a PNG stores as it is: the bytes go in verbatim */
    if ((opened == false) || (format != GUID_WICPixelFormat32bppRGBA)) {
        std::printf("  failed to open %s for writing\n", path.c_str());
        return false;
    }

    auto const saved =
        SUCCEEDED(frame->WritePixels((UINT)height, (UINT)(width * 4), (UINT)pixels.size(), const_cast<BYTE*>(pixels.data()))) &&
        SUCCEEDED(frame->Commit()) &&
        SUCCEEDED(encoder->Commit());

    if (saved == false) {
        std::printf("  failed to save %s\n", path.c_str());
        return false;
    }

    return true;
}

bool _LoadSnapshotPng(std::string const& path, int& width, int& height, std::vector<std::uint8_t>& pixels) {
    auto const factory = _GetSnapshotImagingFactory();
    auto const widePath = std::filesystem::path(path).wstring();
    auto decoder = ComPtr<IWICBitmapDecoder>();
    auto frame = ComPtr<IWICBitmapFrameDecode>();
    auto converter = ComPtr<IWICFormatConverter>();

    if ((factory == nullptr) || FAILED(factory->CreateDecoderFromFilename(widePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder))) {
        std::printf("  missing golden %s\n", path.c_str());
        return false;
    }

    /* to straight RGBA: a PNG holds straight color, so nothing is premultiplied */
    if (
        FAILED(decoder->GetFrame(0, &frame)) ||
        FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))
    ) {
        std::printf("  failed to decode golden %s\n", path.c_str());
        return false;
    }

    auto frameWidth = UINT(0);
    auto frameHeight = UINT(0);
    converter->GetSize(&frameWidth, &frameHeight);

    width  = (int)frameWidth;
    height = (int)frameHeight;

    pixels.resize((std::size_t)width * height * 4);

    if (FAILED(converter->CopyPixels(nullptr, (UINT)(width * 4), (UINT)pixels.size(), pixels.data()))) {
        std::printf("  failed to read golden %s\n", path.c_str());
        return false;
    }

    return true;
}

} /* namespace Rocket */
