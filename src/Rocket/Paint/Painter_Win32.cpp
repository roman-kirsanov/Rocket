/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <map>
#include <array>
#include <cmath>
#include <stack>
#include <string>
#include <tuple>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Painter.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Painter_Private.hpp>
#include <Rocket/Paint/Painter_Win32.hpp>
#include <Rocket/Window/App_Win32.hpp>
#include <Rocket/Window/Window.hpp>
#include "QuadVertex.dxil.hpp"
#include "ColorBrush.dxil.hpp"
#include "ImageBrush.dxil.hpp"
#include "LinearGradientBrush.dxil.hpp"
#include "RadialGradientBrush.dxil.hpp"
#include "BlurFilter.dxil.hpp"
#include "ShadowFilterDilate.dxil.hpp"
#include "ShadowFilterBlur.dxil.hpp"
#include "ShadowFilterComposite.dxil.hpp"

using Microsoft::WRL::ComPtr;

namespace Rocket {

/* The constant buffers of HLSL/Painter.hlsli, laid out as the uniform blocks
   of the other backends (std140): vec2 aligns to 8 bytes, vec4, mat3 columns
   and array elements to 16, so the padding is spelled out and the offsets are
   checked below. The HLSL structs spell out the same padding. */

struct _StateUniforms {
    float resolution[2];
    float _pad0[2];
    float projection[12]; // three columns, each padded to four floats
    float transform[12];  // three columns, each padded to four floats
    float opacity;
    float _pad1[3];
};

struct _ShapeUniforms {
    float vertices[4][4]; // .xy used; vec4 for the std140 array stride
    float bounds[4];
    float borders[4]; // per-edge outline widths (left, top, right, bottom)
    float corners[4]; // per-corner radii (top-left, top-right, bottom-right, bottom-left)
    std::int32_t type; // 0=quad, 1=ellipse, 2=quad outline, 3=ellipse outline
    std::int32_t _pad0[3];
};

struct _ColorBrushUniforms {
    float color[4];
};

struct _ImageBrushUniforms {
    float color[4];
    float size[2];
    float flip[2];
    float repeat[2];
    float _pad0[2];
    float npatch[4];
    float source[4];
    float destin[4];
};

/** Uniforms for the gradient fragment shaders (linear and radial). Colors are premultiplied. */
struct _GradientBrushUniforms {
    float startPoint[2];
    float _pad0[2];
    float startColor[4];
    float stopPoint[2];
    float _pad1[2];
    float stopColor[4];
    std::int32_t stopCount;
    std::int32_t _pad2[3];
    float stopColors[10][4];
    float stopPositions[3][4]; // position i is stopPositions[i / 4][i % 4]
};

struct _BlurFilterUniforms {
    float texelSize[2];
    float direction[2];
    float sigma;
    std::int32_t taps;
    std::int32_t _pad0[2];
};

struct _ShadowFilterUniforms {
    float color[4];
    float opacity;
    float _pad0;
    float texel[2];
    float offset[2];             // inset field displacement, in pixels
    float sigma;                 // gaussian standard deviation in texels
    std::int32_t taps;           // gaussian half-width in taps
    std::int32_t inset;          // 0 = drop shadow, 1 = inset
    std::int32_t fromSilhouette; // 1 = source is the shape texture (.a), 0 = a field texture (.r/.g)
    std::int32_t invert;         // 1 = complement the field (no earlier pass did it)
    std::int32_t _pad1;
};

struct _ShadowFilterFieldUniforms {
    float texel[2];
    float direction[2];          // (1, 0) horizontal pass, (0, 1) vertical pass
    float spread;                // dilate pass: extent in pixels along direction; negative erodes
    float sigma;                 // blur pass: gaussian standard deviation in texels
    std::int32_t taps;           // blur pass: gaussian half-width in taps
    std::int32_t fromSilhouette; // 1 = source is the shape texture (.a), 0 = a field texture (.r/.g)
    std::int32_t invert;         // 1 = complement the field (first pass of an inset chain)
    std::int32_t _pad0[3];
};

static_assert(offsetof(_StateUniforms, projection) == 16);
static_assert(offsetof(_StateUniforms, transform) == 64);
static_assert(offsetof(_StateUniforms, opacity) == 112);
static_assert(offsetof(_ShapeUniforms, bounds) == 64);
static_assert(offsetof(_ShapeUniforms, type) == 112);
static_assert(offsetof(_ImageBrushUniforms, npatch) == 48);
static_assert(offsetof(_ImageBrushUniforms, destin) == 80);
static_assert(offsetof(_GradientBrushUniforms, stopCount) == 64);
static_assert(offsetof(_GradientBrushUniforms, stopColors) == 80);
static_assert(offsetof(_GradientBrushUniforms, stopPositions) == 240);
static_assert(offsetof(_BlurFilterUniforms, sigma) == 16);
static_assert(offsetof(_ShadowFilterUniforms, texel) == 24);
static_assert(offsetof(_ShadowFilterUniforms, invert) == 56);
static_assert(offsetof(_ShadowFilterFieldUniforms, invert) == 32);

enum class _Fragment {
    ColorBrush,
    ImageBrush,
    LinearGradientBrush,
    RadialGradientBrush,
    BlurFilter,
    ShadowFilterDilate,
    ShadowFilterBlur,
    ShadowFilterComposite
};

/** The root parameters, in the order of the root signature. Register spaces are the GLSL descriptor sets. */
enum _RootParameter : UINT {
    _StateVertexParameter = 0,   // b0, space1
    _ShapeVertexParameter,       // b1, space1
    _StatePixelParameter,        // b0, space3
    _BrushPixelParameter,        // b1, space3
    _ShapePixelParameter,        // b2, space3
    _TexturePixelParameter,      // t0, space2
    _SamplerPixelParameter,      // s0, space2
    _RootParameterCount
};

/** The four sampler variants (minification, magnification), in the sampler heap's order. */
enum _Sampler : UINT {
    _NerNerSampler = 0,
    _LinLinSampler,
    _LinNerSampler,
    _NerLinSampler,
    _SamplerCount
};

struct _Texture {
    ComPtr<ID3D12Resource> resource;
    UINT srv = 0;
    UINT rtv = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

struct _UploadPage {
    ComPtr<ID3D12Resource> buffer;
    std::uint8_t* cpu = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS gpu = 0;
    std::uint64_t size = 0;
    std::uint64_t used = 0;
};

struct _UploadAllocation {
    ID3D12Resource* buffer = nullptr;
    std::uint64_t offset = 0;
    std::uint8_t* cpu = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS gpu = 0;
};

/** What a command list has bound, so a draw sets only what changed: every command costs, the software rasterizer's most of all. */
struct _Bindings {
    ID3D12PipelineState* pipeline = nullptr;
    std::optional<D3D12_RECT> scissor;
    std::optional<D3D12_VIEWPORT> viewport;
    UINT srv = UINT_MAX;
    UINT sampler = UINT_MAX;
    D3D12_GPU_VIRTUAL_ADDRESS parameters[_RootParameterCount] = {};
    std::vector<std::uint8_t> uniforms[_RootParameterCount]; // the bytes last uploaded for each parameter
};

/** A command list and what it allocated while being recorded; its pages and allocator are recycled once the GPU has run it. */
struct _Recording {
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12CommandAllocator> allocator;
    std::vector<_UploadPage> pages;
    _Bindings bindings;
    bool open = false;
};

struct _Swapchain {
    ComPtr<IDXGISwapChain3> swapchain;
    ComPtr<ID3D12Resource> buffers[2];
    UINT rtvs[2] = { 0, 0 };
    std::uint64_t fenceValues[2] = { 0, 0 };
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    int references = 0; // passes on it recorded but not yet submitted: it cannot be resized under them
};

/** Something released once the GPU, and every list being recorded when it was released, is done with it. */
struct _Deferred {
    std::uint64_t fence = 0;
    std::function<void()> release;
};

struct _RenderPass {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    ID3D12Resource* texture = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = {};
    D3D12_RESOURCE_STATES restState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // where the texture rests outside its passes
    _Recording* recording = nullptr;
    bool open = false;
    bool commit = false;
};

struct _FilterTextures {
    std::uint64_t lastUse = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::tuple<
        _Texture*,
        _Texture*
    > textures;
};

struct Painter::_Painter {
    _Recording recording;
    std::vector<std::pair<ComPtr<ID3D12CommandAllocator>, std::uint64_t>> allocators;
    std::vector<_Swapchain*> presents; // swapchains to present once the outermost pass is submitted
    std::stack<std::optional<_RenderPass>> renderPassStack;
    std::map<std::uint64_t, _FilterTextures> filterTextures; // keyed by (width << 32) | height
    std::uint64_t filterTexturesUse = 0;
    _FilterTextures& getFilterTextures(std::uint32_t width, std::uint32_t height);
    void evictFilterTextures();
};

/** The device and what every painter and image shares: queue, fence, descriptor heaps, pipelines and recycled memory. */
struct _Context {
    ComPtr<IDXGIFactory4> factory;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent = nullptr;
    std::uint64_t fenceValue = 0; // the last value signaled
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12DescriptorHeap> srvHeap;      // shader visible: one SRV per texture
    ComPtr<ID3D12DescriptorHeap> samplerHeap;  // shader visible: the four samplers
    ComPtr<ID3D12DescriptorHeap> rtvHeap;      // one RTV per texture and per swapchain buffer
    UINT srvSize = 0;
    UINT samplerSize = 0;
    UINT rtvSize = 0;
    std::vector<UINT> freeSrvs;
    std::vector<UINT> freeRtvs;
    UINT nextSrv = 0;
    UINT nextRtv = 0;
    std::map<std::uint64_t, ComPtr<ID3D12PipelineState>> pipelines;
    std::vector<std::pair<_UploadPage, std::uint64_t>> freePages; // with the fence value after which they are free
    _Recording transfer; // uploads, copies and downloads, submitted before any other list
    std::vector<std::pair<ComPtr<ID3D12CommandAllocator>, std::uint64_t>> transferAllocators;
    int openRecordings = 0;
    std::vector<std::function<void()>> waiting; // released while a list was being recorded
    std::vector<_Deferred> deferred;
    std::map<HWND, _Swapchain*> swapchains;
};

static auto constexpr _BLUR_TAP_MAX = 64;
static auto constexpr _FILTER_TEXTURE_CACHE_MAX = 8;
static auto constexpr _SRV_CAPACITY = UINT(65536);
static auto constexpr _RTV_CAPACITY = UINT(65536);
static auto constexpr _UPLOAD_PAGE_SIZE = std::uint64_t(1) << 20;
static auto constexpr _TEXTURE_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
static auto constexpr _SWAPCHAIN_BUFFERS = UINT(2);

static void _Check(HRESULT result, char const* what) {
    PROFILE

    if (FAILED(result)) {
        throw std::runtime_error(std::string(what) + " failed (HRESULT " + std::to_string(static_cast<std::uint32_t>(result)) + ")");
    }
}

static std::uint64_t _Align(std::uint64_t value, std::uint64_t alignment) {
    PROFILE

    return ((value + alignment - 1) / alignment) * alignment;
}

static Vec4 _ResolveBorders(QuadOutlineShape const& shape) {
    PROFILE

    auto const border = shape.border.value_or(0.0f);

    return Vec4{
        shape.leftBorder.value_or(border),
        shape.topBorder.value_or(border),
        shape.rightBorder.value_or(border),
        shape.bottomBorder.value_or(border)
    };
}

template <typename QuadType>
static Vec4 _ResolveRadius(QuadType const& shape) {
    PROFILE

    auto const radius = shape.borderRadius.value_or(0.0f);
    auto const tl = std::max(shape.borderTopLeftRadius.value_or(radius), 0.0f);
    auto const tr = std::max(shape.borderTopRightRadius.value_or(radius), 0.0f);
    auto const br = std::max(shape.borderBottomRightRadius.value_or(radius), 0.0f);
    auto const bl = std::max(shape.borderBottomLeftRadius.value_or(radius), 0.0f);

    auto factor = 1.0f;
    auto const constrain = [&](float side, float sum) {
        if (sum > 0.0f) {
            factor = std::min(factor, (side / sum));
        }
    };

    constrain(shape.rect.width,  (tl + tr));
    constrain(shape.rect.width,  (bl + br));
    constrain(shape.rect.height, (tl + bl));
    constrain(shape.rect.height, (tr + br));

    return Vec4{ (tl * factor), (tr * factor), (br * factor), (bl * factor) };
}

static Vec2 _EdgeExpansion(Mat3 const& transform) {
    PROFILE

    auto const scaleX = std::hypot(transform.data[0], transform.data[1]);
    auto const scaleY = std::hypot(transform.data[3], transform.data[4]);

    return Vec2{
        1.0f / std::max(scaleX, 0.0001f),
        1.0f / std::max(scaleY, 0.0001f)
    };
}

static D3D12_RENDER_TARGET_BLEND_DESC _GetBlend(Blend blend) {
    PROFILE

    auto state = D3D12_RENDER_TARGET_BLEND_DESC{};
    state.BlendEnable = TRUE;
    state.LogicOpEnable = FALSE;
    state.BlendOp = D3D12_BLEND_OP_ADD;
    state.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    state.LogicOp = D3D12_LOGIC_OP_NOOP;
    state.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    switch (blend) {
        case Blend::Over:
            // dst = src + dst * (1 - srcA)
            state.SrcBlend = D3D12_BLEND_ONE;
            state.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            state.SrcBlendAlpha = D3D12_BLEND_ONE;
            state.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            break;
        case Blend::Add:
            // dst = src + dst; the target's alpha is kept
            state.SrcBlend = D3D12_BLEND_ONE;
            state.DestBlend = D3D12_BLEND_ONE;
            state.SrcBlendAlpha = D3D12_BLEND_ZERO;
            state.DestBlendAlpha = D3D12_BLEND_ONE;
            break;
        case Blend::Multiply:
            // dst = src * dst + dst * (1 - srcA): where the source is transparent the target stays
            state.SrcBlend = D3D12_BLEND_DEST_COLOR;
            state.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            state.SrcBlendAlpha = D3D12_BLEND_ZERO;
            state.DestBlendAlpha = D3D12_BLEND_ONE;
            break;
        case Blend::Screen:
            // dst = src + dst * (1 - src)
            state.SrcBlend = D3D12_BLEND_ONE;
            state.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
            state.SrcBlendAlpha = D3D12_BLEND_ZERO;
            state.DestBlendAlpha = D3D12_BLEND_ONE;
            break;
    }

    return state;
}

/* A kernel samples one texel per tap out to its extent, capped at
   _BLUR_TAP_MAX: the power of two a filter's passes are downscaled by to bring
   its widest kernel under the cap */
static float _FilterDownsample(Filter const& filter) {
    PROFILE

    auto const blurFilter = filter.as<BlurFilter>();
    auto const shadowFilter = filter.as<ShadowFilter>();

    auto kernelExtent = 0.0f;
    auto downsample = 1.0f;

    if (blurFilter != nullptr) {
        kernelExtent = blurFilter->getExtent();
    } else if (shadowFilter != nullptr) {
        auto const spread = shadowFilter->spread.value_or(Vec2{});
        kernelExtent = std::max({
            shadowFilter->getExtent(),
            std::abs(spread.x),
            std::abs(spread.y)
        });
    }

    while (
        (kernelExtent / downsample) > static_cast<float>(_BLUR_TAP_MAX)
    ) {
        downsample *= 2.0f;
    }

    return downsample;
}

/* device-space rect a filtered draw can touch: the transformed shape bounds
   grown by what the kernels reach, snapped outward to whole target pixels and
   clipped to the target; nullopt when none of it lands on the target */
static std::optional<Vec4> _FilterBounds(Vec4 const& bounds, Mat3 const& transform, Vec2 const& margin, Vec2 const& target) {
    PROFILE

    auto const aabb   = bounds.toTransformed(transform);
    auto const left   = std::floor(aabb.x - margin.x);
    auto const top    = std::floor(aabb.y - margin.y);
    auto const right  = std::ceil(aabb.getMaxX() + margin.x);
    auto const bottom = std::ceil(aabb.getMaxY() + margin.y);

    if ((std::isfinite(left) && std::isfinite(top) && std::isfinite(right) && std::isfinite(bottom)) == false) {
        return Vec4{ {}, target };
    }

    auto const rect = Vec4{ left, top, (right - left), (bottom - top) }.getIntersection(Vec4{ {}, target });

    if (rect.isEmpty()) {
        return std::nullopt;
    }

    return rect;
}

static Vec4 _ToFilterSpace(Vec4 const& rect, Vec2 const& scale) {
    PROFILE

    auto const left   = std::floor(rect.x * scale.x);
    auto const top    = std::floor(rect.y * scale.y);
    auto const right  = std::ceil(rect.getMaxX() * scale.x);
    auto const bottom = std::ceil(rect.getMaxY() * scale.y);

    return Vec4{ left, top, (right - left), (bottom - top) };
}

static int _GetGaussianTaps(float extent) {
    return std::clamp(static_cast<int>(std::ceil(extent)), 0, _BLUR_TAP_MAX);
}

static ComPtr<IDXGIAdapter1> _GetAdapter(IDXGIFactory4* factory) {
    PROFILE

    /* ROCKET_D3D12_ADAPTER=warp picks the software rasterizer, as the tests can */
    auto const requested = std::getenv("ROCKET_D3D12_ADAPTER");
    auto const warp = (requested != nullptr) && (std::string(requested) == "warp");

    if (warp == false) {
        auto factory6 = ComPtr<IDXGIFactory6>();

        if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6)))) {
            for (auto index = UINT(0); ; index++) {
                auto adapter = ComPtr<IDXGIAdapter1>();

                if (factory6->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) == DXGI_ERROR_NOT_FOUND) {
                    break;
                }

                auto description = DXGI_ADAPTER_DESC1{};
                adapter->GetDesc1(&description);

                /* the Basic Render Driver is the software rasterizer under
                   another name; a GPU without a driver lists it as hardware */
                auto const software = ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
                    || ((description.VendorId == 0x1414) && (description.DeviceId == 0x8C));

                if ((software == false) && SUCCEEDED(::D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
                    return adapter;
                }
            }
        }
    }

    auto adapter = ComPtr<IDXGIAdapter1>();
    _Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)), "EnumWarpAdapter");

    return adapter;
}

static ComPtr<ID3D12RootSignature> _CreateRootSignature(ID3D12Device* device) {
    PROFILE

    auto const srvRange = D3D12_DESCRIPTOR_RANGE{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 2, 0 };
    auto const samplerRange = D3D12_DESCRIPTOR_RANGE{ D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 2, 0 };

    auto const constantBuffer = [](UINT reg, UINT space, D3D12_SHADER_VISIBILITY visibility) {
        auto parameter = D3D12_ROOT_PARAMETER{};
        parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        parameter.Descriptor = D3D12_ROOT_DESCRIPTOR{ reg, space };
        parameter.ShaderVisibility = visibility;
        return parameter;
    };

    auto const table = [](D3D12_DESCRIPTOR_RANGE const* range) {
        auto parameter = D3D12_ROOT_PARAMETER{};
        parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameter.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE{ 1, range };
        parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        return parameter;
    };

    D3D12_ROOT_PARAMETER const parameters[_RootParameterCount] = {
        constantBuffer(0, 1, D3D12_SHADER_VISIBILITY_VERTEX),
        constantBuffer(1, 1, D3D12_SHADER_VISIBILITY_VERTEX),
        constantBuffer(0, 3, D3D12_SHADER_VISIBILITY_PIXEL),
        constantBuffer(1, 3, D3D12_SHADER_VISIBILITY_PIXEL),
        constantBuffer(2, 3, D3D12_SHADER_VISIBILITY_PIXEL),
        table(&srvRange),
        table(&samplerRange)
    };

    auto const description = D3D12_ROOT_SIGNATURE_DESC{ _RootParameterCount, parameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE };
    auto blob = ComPtr<ID3DBlob>();
    auto error = ComPtr<ID3DBlob>();

    _Check(::D3D12SerializeRootSignature(&description, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), "D3D12SerializeRootSignature");

    auto signature = ComPtr<ID3D12RootSignature>();
    _Check(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&signature)), "CreateRootSignature");

    return signature;
}

static ComPtr<ID3D12DescriptorHeap> _CreateHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, bool shaderVisible) {
    PROFILE

    auto const description = D3D12_DESCRIPTOR_HEAP_DESC{
        type,
        count,
        (shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE),
        0
    };

    auto heap = ComPtr<ID3D12DescriptorHeap>();
    _Check(device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&heap)), "CreateDescriptorHeap");

    return heap;
}

static void _CreateSamplers(_Context& context) {
    PROFILE

    auto const filters = std::array<D3D12_FILTER, _SamplerCount>{
        D3D12_FILTER_MIN_MAG_MIP_POINT,              // nearest, nearest
        D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,       // linear, linear
        D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,       // linear, nearest
        D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT  // nearest, linear
    };

    auto handle = context.samplerHeap->GetCPUDescriptorHandleForHeapStart();

    for (auto const filter : filters) {
        auto description = D3D12_SAMPLER_DESC{};
        description.Filter = filter;
        description.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        description.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        description.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        description.MaxAnisotropy = 1;
        description.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        description.MinLOD = 0.0f;
        description.MaxLOD = D3D12_FLOAT32_MAX;

        context.device->CreateSampler(&description, handle);
        handle.ptr += context.samplerSize;
    }
}

static _Context& _GetContext() {
    PROFILE

    static auto const _context = [] {
        App::__InitWin32();

        auto context = new _Context{};

        _Check(::CreateDXGIFactory2(0, IID_PPV_ARGS(&context->factory)), "CreateDXGIFactory2");

        auto const adapter = _GetAdapter(context->factory.Get());
        _Check(::D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&context->device)), "D3D12CreateDevice");

        auto const queueDescription = D3D12_COMMAND_QUEUE_DESC{ D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
        _Check(context->device->CreateCommandQueue(&queueDescription, IID_PPV_ARGS(&context->queue)), "CreateCommandQueue");
        _Check(context->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&context->fence)), "CreateFence");

        context->fenceEvent = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
        context->rootSignature = _CreateRootSignature(context->device.Get());
        context->srvHeap = _CreateHeap(context->device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, _SRV_CAPACITY, true);
        context->samplerHeap = _CreateHeap(context->device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, _SamplerCount, true);
        context->rtvHeap = _CreateHeap(context->device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, _RTV_CAPACITY, false);
        context->srvSize = context->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        context->samplerSize = context->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        context->rtvSize = context->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        _CreateSamplers(*context);

        return context;
    }();

    return *_context;
}

static void _WaitForFence(std::uint64_t value) {
    PROFILE

    auto& context = _GetContext();

    if (context.fence->GetCompletedValue() < value) {
        _Check(context.fence->SetEventOnCompletion(value, context.fenceEvent), "SetEventOnCompletion");
        ::WaitForSingleObject(context.fenceEvent, INFINITE);
    }
}

static std::uint64_t _Signal() {
    PROFILE

    auto& context = _GetContext();

    context.fenceValue++;
    _Check(context.queue->Signal(context.fence.Get(), context.fenceValue), "Signal");

    return context.fenceValue;
}

static void _CollectDeferred() {
    PROFILE

    auto& context = _GetContext();
    auto const completed = context.fence->GetCompletedValue();
    auto due = std::vector<std::function<void()>>();

    std::erase_if(context.deferred, [&](_Deferred& deferred) {
        if (deferred.fence <= completed) {
            due.push_back(std::move(deferred.release));
            return true;
        }
        return false;
    });

    for (auto const& release : due) {
        release();
    }
}

/* What was released while lists were being recorded may be in any of them:
   it waits until none is left, then for the GPU to run the last one. */
static void _RetireWaiting() {
    PROFILE

    auto& context = _GetContext();

    if ((context.openRecordings > 0) || context.waiting.empty()) {
        return;
    }

    for (auto& release : context.waiting) {
        context.deferred.push_back(_Deferred{ context.fenceValue, std::move(release) });
    }

    context.waiting.clear();
}

static void _Defer(std::function<void()> release) {
    PROFILE

    auto& context = _GetContext();

    if (context.openRecordings > 0) {
        context.waiting.push_back(std::move(release));
    } else {
        context.deferred.push_back(_Deferred{ context.fenceValue, std::move(release) });
    }

    _CollectDeferred();
}

static UINT _AllocateSlot(std::vector<UINT>& freeSlots, UINT& next, UINT capacity) {
    PROFILE

    if (freeSlots.empty() == false) {
        auto const slot = freeSlots.back();
        freeSlots.pop_back();
        return slot;
    }

    if (next >= capacity) {
        throw std::runtime_error("Out of texture descriptors");
    }

    return next++;
}

static D3D12_CPU_DESCRIPTOR_HANDLE _GetRtv(UINT slot) {
    PROFILE

    auto& context = _GetContext();
    auto handle = context.rtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(slot) * context.rtvSize;

    return handle;
}

static D3D12_CPU_DESCRIPTOR_HANDLE _GetSrvCpu(UINT slot) {
    PROFILE

    auto& context = _GetContext();
    auto handle = context.srvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(slot) * context.srvSize;

    return handle;
}

static D3D12_GPU_DESCRIPTOR_HANDLE _GetSrvGpu(UINT slot) {
    PROFILE

    auto& context = _GetContext();
    auto handle = context.srvHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(slot) * context.srvSize;

    return handle;
}

static D3D12_GPU_DESCRIPTOR_HANDLE _GetSamplerGpu(_Sampler sampler) {
    PROFILE

    auto& context = _GetContext();
    auto handle = context.samplerHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(sampler) * context.samplerSize;

    return handle;
}

static ComPtr<ID3D12Resource> _CreateBuffer(D3D12_HEAP_TYPE type, std::uint64_t size, D3D12_RESOURCE_STATES state) {
    PROFILE

    auto const heap = D3D12_HEAP_PROPERTIES{ type, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    auto const description = D3D12_RESOURCE_DESC{
        D3D12_RESOURCE_DIMENSION_BUFFER, 0, size, 1, 1, 1,
        DXGI_FORMAT_UNKNOWN, DXGI_SAMPLE_DESC{ 1, 0 },
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE
    };

    auto buffer = ComPtr<ID3D12Resource>();
    _Check(_GetContext().device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description, state, nullptr, IID_PPV_ARGS(&buffer)), "CreateCommittedResource");

    return buffer;
}

static _UploadAllocation _Allocate(_Recording& recording, std::uint64_t size, std::uint64_t alignment) {
    PROFILE

    auto& context = _GetContext();

    if (recording.pages.empty() == false) {
        auto& page = recording.pages.back();
        auto const offset = _Align(page.used, alignment);

        if ((offset + size) <= page.size) {
            page.used = offset + size;
            return _UploadAllocation{ page.buffer.Get(), offset, (page.cpu + offset), (page.gpu + offset) };
        }
    }

    auto const completed = context.fence->GetCompletedValue();
    auto const needed = std::max(_UPLOAD_PAGE_SIZE, _Align(size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));
    auto page = _UploadPage{};

    auto const found = std::ranges::find_if(context.freePages, [&](auto const& entry) {
        return (entry.second <= completed) && (entry.first.size >= needed);
    });

    if (found != context.freePages.end()) {
        page = std::move(found->first);
        context.freePages.erase(found);
    } else {
        page.buffer = _CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, needed, D3D12_RESOURCE_STATE_GENERIC_READ);
        page.size = needed;
        page.gpu = page.buffer->GetGPUVirtualAddress();

        auto const range = D3D12_RANGE{ 0, 0 };
        _Check(page.buffer->Map(0, &range, reinterpret_cast<void**>(&page.cpu)), "Map");
    }

    page.used = size;
    recording.pages.push_back(std::move(page));

    auto& current = recording.pages.back();

    return _UploadAllocation{ current.buffer.Get(), 0, current.cpu, current.gpu };
}

static void _RecyclePages(_Recording& recording, std::uint64_t fence) {
    PROFILE

    auto& context = _GetContext();

    for (auto& page : recording.pages) {
        page.used = 0;
        context.freePages.emplace_back(std::move(page), fence);
    }

    recording.pages.clear();
}

static ComPtr<ID3D12CommandAllocator> _GetAllocator(std::vector<std::pair<ComPtr<ID3D12CommandAllocator>, std::uint64_t>>& allocators) {
    PROFILE

    auto& context = _GetContext();
    auto const completed = context.fence->GetCompletedValue();

    for (auto& [ allocator, fence ] : allocators) {
        if (fence <= completed) {
            fence = UINT64_MAX; // taken
            _Check(allocator->Reset(), "ID3D12CommandAllocator::Reset");
            return allocator;
        }
    }

    auto allocator = ComPtr<ID3D12CommandAllocator>();
    _Check(context.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)), "CreateCommandAllocator");
    allocators.emplace_back(allocator, UINT64_MAX);

    return allocator;
}

static void _ReturnAllocator(std::vector<std::pair<ComPtr<ID3D12CommandAllocator>, std::uint64_t>>& allocators, ID3D12CommandAllocator* allocator, std::uint64_t fence) {
    PROFILE

    for (auto& [ candidate, candidateFence ] : allocators) {
        if (candidate.Get() == allocator) {
            candidateFence = fence;
        }
    }
}

static void _OpenRecording(_Recording& recording, std::vector<std::pair<ComPtr<ID3D12CommandAllocator>, std::uint64_t>>& allocators) {
    PROFILE

    auto& context = _GetContext();

    assert(recording.open == false);

    recording.allocator = _GetAllocator(allocators);

    if (recording.list == nullptr) {
        _Check(context.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, recording.allocator.Get(), nullptr, IID_PPV_ARGS(&recording.list)), "CreateCommandList");
    } else {
        _Check(recording.list->Reset(recording.allocator.Get(), nullptr), "ID3D12GraphicsCommandList::Reset");
    }

    ID3D12DescriptorHeap* const heaps[] = { context.srvHeap.Get(), context.samplerHeap.Get() };
    recording.list->SetDescriptorHeaps(2, heaps);
    recording.list->SetGraphicsRootSignature(context.rootSignature.Get());
    recording.bindings = _Bindings{}; /* a reset list has nothing bound */
    recording.open = true;
    context.openRecordings++;
}

/* Closes the list and runs it; returns the fence value it is done at. */
static std::uint64_t _SubmitRecording(_Recording& recording, std::vector<std::pair<ComPtr<ID3D12CommandAllocator>, std::uint64_t>>& allocators) {
    PROFILE

    auto& context = _GetContext();

    assert(recording.open);

    _Check(recording.list->Close(), "ID3D12GraphicsCommandList::Close");

    ID3D12CommandList* const lists[] = { recording.list.Get() };
    context.queue->ExecuteCommandLists(1, lists);

    auto const fence = _Signal();

    _ReturnAllocator(allocators, recording.allocator.Get(), fence);
    _RecyclePages(recording, fence);

    recording.allocator.Reset();
    recording.open = false;
    context.openRecordings--;

    return fence;
}

/* Drops a list that was never run: what it allocated is free at once. */
static void _DiscardRecording(_Recording& recording, std::vector<std::pair<ComPtr<ID3D12CommandAllocator>, std::uint64_t>>& allocators) {
    PROFILE

    auto& context = _GetContext();

    assert(recording.open);

    recording.list->Close();

    _ReturnAllocator(allocators, recording.allocator.Get(), 0);
    _RecyclePages(recording, 0);

    recording.allocator.Reset();
    recording.open = false;
    context.openRecordings--;
}

static _Recording& _GetTransfer() {
    PROFILE

    auto& context = _GetContext();

    if (context.transfer.open == false) {
        _OpenRecording(context.transfer, context.transferAllocators);
    }

    return context.transfer;
}

/* Runs the pending uploads and copies, before anything submitted after them. */
static void _FlushTransfer() {
    PROFILE

    auto& context = _GetContext();

    if (context.transfer.open) {
        _SubmitRecording(context.transfer, context.transferAllocators);
        _RetireWaiting();
    }
}

static void _Barrier(ID3D12GraphicsCommandList* list, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    PROFILE

    if (before == after) {
        return;
    }

    auto barrier = D3D12_RESOURCE_BARRIER{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;

    list->ResourceBarrier(1, &barrier);
}

static D3D12_SHADER_BYTECODE _GetPixelShader(_Fragment fragment) {
    PROFILE

    switch (fragment) {
        case _Fragment::ColorBrush:            return { Rocket_DXIL_ColorBrush, Rocket_DXIL_ColorBrush_len };
        case _Fragment::ImageBrush:            return { Rocket_DXIL_ImageBrush, Rocket_DXIL_ImageBrush_len };
        case _Fragment::LinearGradientBrush:   return { Rocket_DXIL_LinearGradientBrush, Rocket_DXIL_LinearGradientBrush_len };
        case _Fragment::RadialGradientBrush:   return { Rocket_DXIL_RadialGradientBrush, Rocket_DXIL_RadialGradientBrush_len };
        case _Fragment::BlurFilter:            return { Rocket_DXIL_BlurFilter, Rocket_DXIL_BlurFilter_len };
        case _Fragment::ShadowFilterDilate:    return { Rocket_DXIL_ShadowFilterDilate, Rocket_DXIL_ShadowFilterDilate_len };
        case _Fragment::ShadowFilterBlur:      return { Rocket_DXIL_ShadowFilterBlur, Rocket_DXIL_ShadowFilterBlur_len };
        case _Fragment::ShadowFilterComposite: return { Rocket_DXIL_ShadowFilterComposite, Rocket_DXIL_ShadowFilterComposite_len };
    }

    return {};
}

static ComPtr<ID3D12PipelineState> _CreatePipeline(_Fragment fragment, DXGI_FORMAT format, Blend blend) {
    PROFILE

    auto& context = _GetContext();
    auto description = D3D12_GRAPHICS_PIPELINE_STATE_DESC{};

    description.pRootSignature = context.rootSignature.Get();
    description.VS = D3D12_SHADER_BYTECODE{ Rocket_DXIL_QuadVertex, Rocket_DXIL_QuadVertex_len };
    description.PS = _GetPixelShader(fragment);
    description.BlendState.RenderTarget[0] = _GetBlend(blend);
    description.SampleMask = UINT_MAX;
    description.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    description.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    description.RasterizerState.DepthClipEnable = TRUE;
    description.DepthStencilState.DepthEnable = FALSE;
    description.DepthStencilState.StencilEnable = FALSE;
    description.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    description.NumRenderTargets = 1;
    description.RTVFormats[0] = format;
    description.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };

    auto pipeline = ComPtr<ID3D12PipelineState>();
    _Check(context.device->CreateGraphicsPipelineState(&description, IID_PPV_ARGS(&pipeline)), "CreateGraphicsPipelineState");

    return pipeline;
}

static ID3D12PipelineState* _GetPipeline(_Fragment fragment, DXGI_FORMAT format, Blend blend) {
    PROFILE

    auto& context = _GetContext();
    auto const key = (((std::uint64_t)fragment) << 40) | (((std::uint64_t)format) << 8) | (std::uint64_t)blend;
    auto it = context.pipelines.find(key);

    if (it == context.pipelines.end()) {
        it = context.pipelines.emplace(key, _CreatePipeline(fragment, format, blend)).first;
    }

    return it->second.Get();
}

static ID3D12GraphicsCommandList* _GetList(_RenderPass const& renderPass) {
    PROFILE

    assert(renderPass.recording != nullptr);
    assert(renderPass.open);

    return renderPass.recording->list.Get();
}

static void _SetScissor(_RenderPass const& renderPass, std::optional<Vec4> const& scissor) {
    PROFILE

    auto const passWidth = static_cast<int>(renderPass.width);
    auto const passHeight = static_cast<int>(renderPass.height);

    auto rect = D3D12_RECT{ 0, 0, passWidth, passHeight };

    if (scissor.has_value()) {
        auto const left = std::max(0, static_cast<int>(std::roundf(scissor->x)));
        auto const top = std::max(0, static_cast<int>(std::roundf(scissor->y)));
        auto const right = std::min(passWidth, static_cast<int>(std::roundf(scissor->x + scissor->width)));
        auto const bottom = std::min(passHeight, static_cast<int>(std::roundf(scissor->y + scissor->height)));

        rect.left = std::min(left, passWidth);
        rect.top = std::min(top, passHeight);
        rect.right = rect.left + std::max(0, right - left);
        rect.bottom = rect.top + std::max(0, bottom - top);
    }

    auto& bound = renderPass.recording->bindings.scissor;

    if (
        (bound.has_value() == false) ||
        (bound->left != rect.left) || (bound->top != rect.top) ||
        (bound->right != rect.right) || (bound->bottom != rect.bottom)
    ) {
        _GetList(renderPass)->RSSetScissorRects(1, &rect);
        bound = rect;
    }
}

static void _SetViewport(_RenderPass const& renderPass, Vec4 const& viewport) {
    PROFILE

    auto const viewportInfo = D3D12_VIEWPORT{
        viewport.x,
        viewport.y,
        viewport.width,
        viewport.height,
        0.0f,
        1.0f
    };

    auto& bound = renderPass.recording->bindings.viewport;

    if ((bound.has_value() == false) || (std::memcmp(&*bound, &viewportInfo, sizeof(D3D12_VIEWPORT)) != 0)) {
        _GetList(renderPass)->RSSetViewports(1, &viewportInfo);
        bound = viewportInfo;
    }
}

static void _SetPipeline(_RenderPass const& renderPass, ID3D12PipelineState* pipeline) {
    PROFILE

    auto& bound = renderPass.recording->bindings.pipeline;

    if (bound != pipeline) {
        _GetList(renderPass)->SetPipelineState(pipeline);
        bound = pipeline;
    }
}

static void _BindTexture(_RenderPass const& renderPass, _Texture const* texture, _Sampler sampler) {
    PROFILE

    assert(texture != nullptr);

    auto const list = _GetList(renderPass);
    auto& bindings = renderPass.recording->bindings;

    if (bindings.srv != texture->srv) {
        list->SetGraphicsRootDescriptorTable(_TexturePixelParameter, _GetSrvGpu(texture->srv));
        bindings.srv = texture->srv;
    }

    if (bindings.sampler != sampler) {
        list->SetGraphicsRootDescriptorTable(_SamplerPixelParameter, _GetSamplerGpu(sampler));
        bindings.sampler = sampler;
    }
}

static void _BindUniforms(_RenderPass const& renderPass, D3D12_GPU_VIRTUAL_ADDRESS address, std::initializer_list<UINT> parameters) {
    PROFILE

    auto& bound = renderPass.recording->bindings.parameters;

    for (auto const parameter : parameters) {
        if (bound[parameter] != address) {
            _GetList(renderPass)->SetGraphicsRootConstantBufferView(parameter, address);
            bound[parameter] = address;
        }
    }
}

/* Uploads one uniform block for this draw and binds it to the given root
   parameters. Successive draws often push the same block (the state, a glyph
   color): then the one already uploaded stays bound. */
template <typename Uniforms>
static void _PushUniforms(_RenderPass const& renderPass, Uniforms const& uniforms, std::initializer_list<UINT> parameters) {
    PROFILE

    auto& bindings = renderPass.recording->bindings;
    auto const bytes = reinterpret_cast<std::uint8_t const*>(&uniforms);
    auto const first = *parameters.begin();
    auto& last = bindings.uniforms[first];

    auto const same =
        (last.size() == sizeof(Uniforms)) &&
        (std::memcmp(last.data(), bytes, sizeof(Uniforms)) == 0) &&
        std::ranges::all_of(parameters, [&](UINT parameter) { return bindings.parameters[parameter] == bindings.parameters[first]; });

    if (same) {
        return;
    }

    auto const allocation = _Allocate(*renderPass.recording, sizeof(Uniforms), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    std::memcpy(allocation.cpu, bytes, sizeof(Uniforms));

    last.assign(bytes, (bytes + sizeof(Uniforms)));
    _BindUniforms(renderPass, allocation.gpu, parameters);
}

static void _OpenRenderPass(_RenderPass& renderPass, std::optional<Vec4> const& clear) {
    PROFILE

    auto const list = renderPass.recording->list.Get();

    _Barrier(list, renderPass.texture, renderPass.restState, D3D12_RESOURCE_STATE_RENDER_TARGET);
    list->OMSetRenderTargets(1, &renderPass.rtv, FALSE, nullptr);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    if (clear.has_value()) {
        /* the target is premultiplied; the clear color is straight RGBA */
        float const color[4] = {
            (clear->red * clear->alpha),
            (clear->green * clear->alpha),
            (clear->blue * clear->alpha),
            clear->alpha
        };

        list->ClearRenderTargetView(renderPass.rtv, color, 0, nullptr);
    }

    renderPass.open = true;
}

static void _CloseRenderPass(_RenderPass& renderPass) {
    PROFILE

    _Barrier(renderPass.recording->list.Get(), renderPass.texture, D3D12_RESOURCE_STATE_RENDER_TARGET, renderPass.restState);
    renderPass.open = false;
}

static void _BeginRenderPass(_RenderPass& renderPass, _Recording& recording, _Texture const* texture, std::optional<Vec4> const& clear) {
    PROFILE

    assert(texture != nullptr);
    assert(renderPass.open == false);

    renderPass.width = texture->width;
    renderPass.height = texture->height;
    renderPass.format = _TEXTURE_FORMAT;
    renderPass.texture = texture->resource.Get();
    renderPass.rtv = _GetRtv(texture->rtv);
    renderPass.restState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    renderPass.recording = &recording;

    _OpenRenderPass(renderPass, clear);
}

static void _EndRenderPass(_RenderPass& renderPass) {
    PROFILE

    assert(renderPass.open);

    _CloseRenderPass(renderPass);

    renderPass.width = 0;
    renderPass.height = 0;
    renderPass.texture = nullptr;
    renderPass.recording = nullptr;
    renderPass.format = DXGI_FORMAT_UNKNOWN;
}

/* A suspended target goes back to its resting state, so a nested pass can sample it. */
static void _SuspendRenderPass(_RenderPass& renderPass) {
    PROFILE

    assert(renderPass.open);

    _CloseRenderPass(renderPass);
}

static void _ResumeRenderPass(_RenderPass& renderPass) {
    PROFILE

    assert(renderPass.open == false);

    _OpenRenderPass(renderPass, std::nullopt);
}

static void _CopyColumn(float* column, float x, float y, float z) {
    column[0] = x;
    column[1] = y;
    column[2] = z;
    column[3] = 0.0f;
}

static void _PushStateUniforms(_RenderPass const& renderPass, Vec2 const& resolution, Mat3 const& transform, float opacity) {
    PROFILE

    auto stateUniforms = _StateUniforms{};
    stateUniforms.resolution[0] = resolution.x;
    stateUniforms.resolution[1] = resolution.y;
    _CopyColumn(&stateUniforms.projection[0], (2.0f / resolution.width), 0.0f, 0.0f);
    _CopyColumn(&stateUniforms.projection[4], 0.0f, (-2.0f / resolution.height), 0.0f);
    _CopyColumn(&stateUniforms.projection[8], -1.0f, 1.0f, 1.0f);
    _CopyColumn(&stateUniforms.transform[0], transform.data[0], transform.data[1], transform.data[2]);
    _CopyColumn(&stateUniforms.transform[4], transform.data[3], transform.data[4], transform.data[5]);
    _CopyColumn(&stateUniforms.transform[8], transform.data[6], transform.data[7], transform.data[8]);
    stateUniforms.opacity = opacity;

    _PushUniforms(renderPass, stateUniforms, { _StateVertexParameter, _StatePixelParameter });
}

static void _PushShapeUniforms(_RenderPass const& renderPass, Shape const& shape, Vec2 const& expand, bool fragment) {
    PROFILE

    auto rect    = Vec4{};
    auto borders = Vec4{};
    auto corners = Vec4{};
    auto type    = 0;

    shape.match(
        [&](QuadShape const& quadShape) {
            rect    = quadShape.rect;
            corners = _ResolveRadius(quadShape);
            type    = 0;
        },
        [&](EllipseShape const& ellipseShape) {
            rect = ellipseShape.rect;
            type = 1;
        },
        [&](QuadOutlineShape const& quadOutlineShape) {
            rect    = quadOutlineShape.rect;
            borders = _ResolveBorders(quadOutlineShape);
            corners = _ResolveRadius(quadOutlineShape);
            type    = 2;
        },
        [&](EllipseOutlineShape const& ellipseOutlineShape) {
            auto const width = ellipseOutlineShape.border.value_or(0.0f);
            rect    = ellipseOutlineShape.rect;
            borders = Vec4{ width, width, width, width };
            type    = 3;
        }
    );

    auto const shapeUniforms = _ShapeUniforms{
        .vertices = {
            { (rect.x - expand.x),         (rect.y - expand.y),         0.0f, 0.0f },
            { (rect.x - expand.x),         (rect.getMaxY() + expand.y), 0.0f, 0.0f },
            { (rect.getMaxX() + expand.x), (rect.y - expand.y),         0.0f, 0.0f },
            { (rect.getMaxX() + expand.x), (rect.getMaxY() + expand.y), 0.0f, 0.0f }
        },
        .bounds  = { rect.x, rect.y, rect.width, rect.height }, // unexpanded: localPos and localUV stay in shape space
        .borders = { borders.left, borders.top, borders.right, borders.bottom },
        .corners = { corners.data[0], corners.data[1], corners.data[2], corners.data[3] },
        .type    = type
    };

    if (fragment) {
        _PushUniforms(renderPass, shapeUniforms, { _ShapeVertexParameter, _ShapePixelParameter });
    } else {
        _PushUniforms(renderPass, shapeUniforms, { _ShapeVertexParameter });
    }
}

static void _PushColorBrushUniforms(_RenderPass const& renderPass, ColorBrush const& colorBrush) {
    PROFILE

    auto const colorUniforms = _ColorBrushUniforms{
        .color = { colorBrush.color.red, colorBrush.color.green, colorBrush.color.blue, colorBrush.color.alpha }
    };

    _PushUniforms(renderPass, colorUniforms, { _BrushPixelParameter });
}

static void _PushImageBrushUniforms(_RenderPass const& renderPass, ImageBrush const& imageBrush, Vec4 const& bounds) {
    PROFILE

    assert(imageBrush.image != nullptr);

    auto const imageSize = imageBrush.image->getSize();
    auto const imageColor = imageBrush.color.value_or(Vec4{});
    auto const imageNPatch = imageBrush.nPatch.value_or(Vec4{});
    auto const filterMin = imageBrush.filterMin.value_or(ImageFilter::Nearest);
    auto const filterMag = imageBrush.filterMag.value_or(ImageFilter::Nearest);
    auto const positionX = imageBrush.positionX.value_or(ImagePosition::Stretch);
    auto const positionY = imageBrush.positionY.value_or(ImagePosition::Stretch);
    auto const offsetX = imageBrush.offsetX.value_or(0.0f);
    auto const offsetY = imageBrush.offsetY.value_or(0.0f);
    auto const repeatX = imageBrush.repeatX.value_or(false);
    auto const repeatY = imageBrush.repeatY.value_or(false);
    auto const flipX = imageBrush.flipX.value_or(false);
    auto const flipY = imageBrush.flipY.value_or(false);

    auto imageSrc = Vec4{ {}, imageSize };
    auto imageDst = Vec4{ {}, imageSize };

    if (imageBrush.slice.has_value()) {
        imageSrc = imageBrush.slice.value();
        imageDst.width = imageSrc.width;
        imageDst.height = imageSrc.height;
    }

    if (imageBrush.fit == true) {
        auto const factor = std::min(
            (bounds.width / imageDst.width),
            (bounds.height / imageDst.height)
        );

        if (factor < 1.0f) {
            imageDst.size *= factor;
        }
    }

    if (positionX == ImagePosition::Center) {
        imageDst.x = ((bounds.width - imageDst.width) / 2.0f);
    } else if (positionX == ImagePosition::End) {
        imageDst.x = (bounds.width - imageDst.width);
    } else if (positionX == ImagePosition::Stretch) {
        imageDst.width = bounds.width;
    }

    if (positionY == ImagePosition::Center) {
        imageDst.y = ((bounds.height - imageDst.height) / 2.0f);
    } else if (positionY == ImagePosition::End) {
        imageDst.y = (bounds.height - imageDst.height);
    } else if (positionY == ImagePosition::Stretch) {
        imageDst.height = bounds.height;
    }

    // A repeated axis wraps the offset by one tile: the tiles look the same,
    // and the shader keeps its float precision however large the offset grows.
    imageDst.x += ((repeatX && (imageDst.width > 0.0f)) ? std::fmod(offsetX, imageDst.width) : offsetX);
    imageDst.y += ((repeatY && (imageDst.height > 0.0f)) ? std::fmod(offsetY, imageDst.height) : offsetY);

    auto const imageUniforms = _ImageBrushUniforms{
        .color  = { imageColor.red, imageColor.green, imageColor.blue, imageColor.alpha },
        .size   = { imageSize.width, imageSize.height },
        .flip   = { (flipX ? 1.0f : 0.0f), (flipY ? 1.0f : 0.0f) },
        .repeat = { (repeatX ? 1.0f : 0.0f), (repeatY ? 1.0f : 0.0f) },
        ._pad0  = {},
        .npatch = { imageNPatch.left, imageNPatch.top, imageNPatch.right, imageNPatch.bottom },
        .source = { imageSrc.x, imageSrc.y, imageSrc.getMaxX(), imageSrc.getMaxY() },
        .destin = { imageDst.x, imageDst.y, imageDst.getMaxX(), imageDst.getMaxY() }
    };

    auto sampler = _NerNerSampler;

    if (filterMin == ImageFilter::Linear && filterMag == ImageFilter::Linear) {
        sampler = _LinLinSampler;
    } else if (filterMin == ImageFilter::Linear && filterMag == ImageFilter::Nearest) {
        sampler = _LinNerSampler;
    } else if (filterMin == ImageFilter::Nearest && filterMag == ImageFilter::Linear) {
        sampler = _NerLinSampler;
    }

    _BindTexture(renderPass, static_cast<_Texture const*>(imageBrush.image->getTexture()), sampler);
    _PushUniforms(renderPass, imageUniforms, { _BrushPixelParameter });
}

static void _PushGradientBrushUniforms(_RenderPass const& renderPass, GradientBrush const& gradientBrush) {
    PROFILE

    auto const startPosition = gradientBrush.startPosition.value_or(Vec2{ 0.0f, 0.0f });
    auto const stopPosition = gradientBrush.stopPosition.value_or(Vec2{ 1.0f, 1.0f });
    auto const stopCount = std::min<int>(gradientBrush.stops.size(), 10);
    auto startColor = gradientBrush.startColor.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });
    auto stopColor = gradientBrush.stopColor.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });

    if (stopCount > 0) {
        startColor = std::get<1>(gradientBrush.stops.front());
        stopColor  = std::get<1>(gradientBrush.stops[stopCount - 1]);
    }

    // Stops are premultiplied here so the shader interpolates in premultiplied
    // space: a fade to transparent then loses coverage without darkening.
    auto const premultiply = [](float* out, Vec4 const& c) {
        out[0] = (c.red * c.alpha);
        out[1] = (c.green * c.alpha);
        out[2] = (c.blue * c.alpha);
        out[3] = c.alpha;
    };

    auto gradientUniforms = _GradientBrushUniforms{};
    gradientUniforms.startPoint[0] = startPosition.x;
    gradientUniforms.startPoint[1] = startPosition.y;
    gradientUniforms.stopPoint[0] = stopPosition.x;
    gradientUniforms.stopPoint[1] = stopPosition.y;
    gradientUniforms.stopCount = stopCount;
    premultiply(gradientUniforms.startColor, startColor);
    premultiply(gradientUniforms.stopColor, stopColor);

    for (auto i = 0; i < stopCount; i++) {
        auto const& [ position, color ] = gradientBrush.stops[i];
        premultiply(gradientUniforms.stopColors[i], color);
        gradientUniforms.stopPositions[i / 4][i % 4] = position;
    }

    _PushUniforms(renderPass, gradientUniforms, { _BrushPixelParameter });
}

static void _PushBlurFilterUniforms(_RenderPass const& renderPass, Vec2 const& texelSize, Vec2 const& direction, BlurFilter const& filter, Vec2 const& scale) {
    PROFILE

    auto const axisScale = ((direction.x != 0.0f) ? scale.x : scale.y);

    auto const blurUniforms = _BlurFilterUniforms{
        .texelSize = { texelSize.x, texelSize.y },
        .direction = { direction.x, direction.y },
        .sigma     = (filter.getSigma() * axisScale),
        .taps      = _GetGaussianTaps(filter.getExtent() * axisScale),
        ._pad0     = {}
    };

    _PushUniforms(renderPass, blurUniforms, { _BrushPixelParameter });
}

static void _PushShadowFilterFieldUniforms(_RenderPass const& renderPass, Vec2 const& texel, Vec2 const& direction, float spread, ShadowFilter const& filter, bool fromSilhouette, Vec2 const& scale) {
    PROFILE

    auto const axisScale = ((direction.x != 0.0f) ? scale.x : scale.y);

    auto const fieldUniforms = _ShadowFilterFieldUniforms{
        .texel          = { texel.x, texel.y },
        .direction      = { direction.x, direction.y },
        .spread         = std::clamp((spread * axisScale), -static_cast<float>(_BLUR_TAP_MAX), static_cast<float>(_BLUR_TAP_MAX)),
        .sigma          = (filter.getSigma() * axisScale),
        .taps           = _GetGaussianTaps(filter.getExtent() * axisScale),
        .fromSilhouette = (fromSilhouette ? 1 : 0),
        .invert         = ((fromSilhouette && filter.inset) ? 1 : 0),
        ._pad0          = {}
    };

    _PushUniforms(renderPass, fieldUniforms, { _BrushPixelParameter });
}

static void _PushShadowFilterUniforms(_RenderPass const& renderPass, Vec2 const& texel, ShadowFilter const& filter, bool fromSilhouette, Vec2 const& scale) {
    PROFILE

    auto const offset = filter.offset.value_or(Vec2{});

    auto const shadowUniforms = _ShadowFilterUniforms{
        .color          = { filter.color.red, filter.color.green, filter.color.blue, filter.color.alpha },
        .opacity        = 1.0f,
        ._pad0          = 0.0f,
        .texel          = { texel.x, texel.y },
        .offset         = { (offset.x * scale.x), (offset.y * scale.y) },
        .sigma          = (filter.getSigma() * scale.y),
        .taps           = _GetGaussianTaps(filter.getExtent() * scale.y),
        .inset          = (filter.inset ? 1 : 0),
        .fromSilhouette = (fromSilhouette ? 1 : 0),
        .invert         = ((fromSilhouette && filter.inset) ? 1 : 0),
        ._pad1          = 0
    };

    _PushUniforms(renderPass, shadowUniforms, { _BrushPixelParameter });
}

static void _Draw(_RenderPass const& renderPass) {
    PROFILE

    _GetList(renderPass)->DrawInstanced(4, 1, 0, 0);
}

void* __GetDefaultGPUDevice() {
    PROFILE

    return _GetContext().device.Get();
}

int __GetDefaultTextureFormat() {
    PROFILE

    return (int)_TEXTURE_FORMAT;
}

void* __CreateTexture(std::uint32_t width, std::uint32_t height) {
    PROFILE

    auto& context = _GetContext();
    auto texture = new _Texture{};

    texture->width = std::max(1u, width);
    texture->height = std::max(1u, height);

    auto const heap = D3D12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    auto const description = D3D12_RESOURCE_DESC{
        D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, texture->width, texture->height, 1, 1,
        _TEXTURE_FORMAT, DXGI_SAMPLE_DESC{ 1, 0 },
        D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
    };
    auto const clear = D3D12_CLEAR_VALUE{ _TEXTURE_FORMAT, { 0.0f, 0.0f, 0.0f, 0.0f } };

    _Check(context.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description, D3D12_RESOURCE_STATE_RENDER_TARGET, &clear, IID_PPV_ARGS(&texture->resource)), "CreateCommittedResource");

    texture->srv = _AllocateSlot(context.freeSrvs, context.nextSrv, _SRV_CAPACITY);
    texture->rtv = _AllocateSlot(context.freeRtvs, context.nextRtv, _RTV_CAPACITY);

    context.device->CreateShaderResourceView(texture->resource.Get(), nullptr, _GetSrvCpu(texture->srv));
    context.device->CreateRenderTargetView(texture->resource.Get(), nullptr, _GetRtv(texture->rtv));

    /* a new texture's content is undefined: it starts transparent, and rests
       in the state it is sampled in */
    auto& transfer = _GetTransfer();
    float const transparent[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    transfer.list->ClearRenderTargetView(_GetRtv(texture->rtv), transparent, 0, nullptr);
    _Barrier(transfer.list.Get(), texture->resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    return texture;
}

void __ReleaseTexture(void* handle) {
    PROFILE

    if (handle == nullptr) {
        return;
    }

    auto const texture = static_cast<_Texture*>(handle);

    _Defer([texture]() {
        auto& context = _GetContext();
        context.freeSrvs.push_back(texture->srv);
        context.freeRtvs.push_back(texture->rtv);
        delete texture;
    });
}

void __UploadTexture(void* handle, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, std::uint8_t const* pixels) {
    PROFILE

    if ((width == 0) || (height == 0)) {
        return;
    }

    auto const texture = static_cast<_Texture*>(handle);
    auto& transfer = _GetTransfer();

    auto const rowBytes = static_cast<std::uint64_t>(width) * 4;
    auto const rowPitch = _Align(rowBytes, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    auto const allocation = _Allocate(transfer, (rowPitch * height), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

    for (auto row = std::uint32_t(0); row < height; row++) {
        std::memcpy((allocation.cpu + (row * rowPitch)), (pixels + (row * rowBytes)), rowBytes);
    }

    auto source = D3D12_TEXTURE_COPY_LOCATION{};
    source.pResource = allocation.buffer;
    source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    source.PlacedFootprint.Offset = allocation.offset;
    source.PlacedFootprint.Footprint = D3D12_SUBRESOURCE_FOOTPRINT{ _TEXTURE_FORMAT, width, height, 1, static_cast<UINT>(rowPitch) };

    auto destination = D3D12_TEXTURE_COPY_LOCATION{};
    destination.pResource = texture->resource.Get();
    destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destination.SubresourceIndex = 0;

    _Barrier(transfer.list.Get(), texture->resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    transfer.list->CopyTextureRegion(&destination, x, y, 0, &source, nullptr);
    _Barrier(transfer.list.Get(), texture->resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void __DownloadTexture(void* handle, std::vector<std::uint8_t>& pixels) {
    PROFILE

    auto const texture = static_cast<_Texture*>(handle);
    auto& transfer = _GetTransfer();

    auto const rowBytes = static_cast<std::uint64_t>(texture->width) * 4;
    auto const rowPitch = _Align(rowBytes, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    auto const readback = _CreateBuffer(D3D12_HEAP_TYPE_READBACK, (rowPitch * texture->height), D3D12_RESOURCE_STATE_COPY_DEST);

    auto source = D3D12_TEXTURE_COPY_LOCATION{};
    source.pResource = texture->resource.Get();
    source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    source.SubresourceIndex = 0;

    auto destination = D3D12_TEXTURE_COPY_LOCATION{};
    destination.pResource = readback.Get();
    destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    destination.PlacedFootprint.Offset = 0;
    destination.PlacedFootprint.Footprint = D3D12_SUBRESOURCE_FOOTPRINT{ _TEXTURE_FORMAT, texture->width, texture->height, 1, static_cast<UINT>(rowPitch) };

    _Barrier(transfer.list.Get(), texture->resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
    transfer.list->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
    _Barrier(transfer.list.Get(), texture->resource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    _FlushTransfer();
    _WaitForFence(_GetContext().fenceValue);

    auto mapped = static_cast<std::uint8_t*>(nullptr);
    auto const range = D3D12_RANGE{ 0, static_cast<SIZE_T>(rowPitch * texture->height) };

    _Check(readback->Map(0, &range, reinterpret_cast<void**>(&mapped)), "Map");

    /* rows are padded to the pitch the GPU copies them with: packed tight here */
    pixels.resize(static_cast<std::size_t>(rowBytes * texture->height));

    for (auto row = std::uint32_t(0); row < texture->height; row++) {
        std::memcpy((pixels.data() + (row * rowBytes)), (mapped + (row * rowPitch)), rowBytes);
    }

    auto const written = D3D12_RANGE{ 0, 0 };
    readback->Unmap(0, &written);

    _CollectDeferred();
}

void __CopyTexture(void* sourceHandle, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, void* destinationHandle, std::uint32_t toX, std::uint32_t toY) {
    PROFILE

    if ((width == 0) || (height == 0)) {
        return;
    }

    auto const sourceTexture = static_cast<_Texture*>(sourceHandle);
    auto const destinationTexture = static_cast<_Texture*>(destinationHandle);
    auto& transfer = _GetTransfer();

    auto source = D3D12_TEXTURE_COPY_LOCATION{};
    source.pResource = sourceTexture->resource.Get();
    source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    source.SubresourceIndex = 0;

    auto destination = D3D12_TEXTURE_COPY_LOCATION{};
    destination.pResource = destinationTexture->resource.Get();
    destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destination.SubresourceIndex = 0;

    auto const box = D3D12_BOX{ x, y, 0, (x + width), (y + height), 1 };

    _Barrier(transfer.list.Get(), sourceTexture->resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
    _Barrier(transfer.list.Get(), destinationTexture->resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    transfer.list->CopyTextureRegion(&destination, toX, toY, 0, &source, &box);
    _Barrier(transfer.list.Get(), destinationTexture->resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    _Barrier(transfer.list.Get(), sourceTexture->resource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

/* Lets go of a swapchain's buffers, which ResizeBuffers and releasing need. */
static void _ReleaseSwapchainBuffers(_Swapchain& swapchain) {
    PROFILE

    auto& context = _GetContext();

    _WaitForFence(context.fenceValue);

    for (auto i = UINT(0); i < _SWAPCHAIN_BUFFERS; i++) {
        swapchain.buffers[i].Reset();
        swapchain.fenceValues[i] = 0;
    }
}

static void _AcquireSwapchainBuffers(_Swapchain& swapchain) {
    PROFILE

    auto& context = _GetContext();

    for (auto i = UINT(0); i < _SWAPCHAIN_BUFFERS; i++) {
        _Check(swapchain.swapchain->GetBuffer(i, IID_PPV_ARGS(&swapchain.buffers[i])), "IDXGISwapChain::GetBuffer");
        context.device->CreateRenderTargetView(swapchain.buffers[i].Get(), nullptr, _GetRtv(swapchain.rtvs[i]));
    }
}

/* The window's swapchain at the size of its content area, made on its first
   pass; nullptr while there is nothing to present to (a minimized window). */
static _Swapchain* _GetSwapchain(HWND hwnd) {
    PROFILE

    auto& context = _GetContext();
    auto client = RECT{};

    ::GetClientRect(hwnd, &client);

    auto const width = static_cast<std::uint32_t>(std::max(0L, client.right));
    auto const height = static_cast<std::uint32_t>(std::max(0L, client.bottom));

    if ((width == 0) || (height == 0)) {
        return nullptr;
    }

    auto it = context.swapchains.find(hwnd);

    if (it == context.swapchains.end()) {
        auto swapchain = new _Swapchain{};
        auto const description = DXGI_SWAP_CHAIN_DESC1{
            width, height, _TEXTURE_FORMAT, FALSE, DXGI_SAMPLE_DESC{ 1, 0 },
            DXGI_USAGE_RENDER_TARGET_OUTPUT, _SWAPCHAIN_BUFFERS, DXGI_SCALING_STRETCH,
            DXGI_SWAP_EFFECT_FLIP_DISCARD, DXGI_ALPHA_MODE_IGNORE, 0
        };
        auto swapchain1 = ComPtr<IDXGISwapChain1>();

        _Check(context.factory->CreateSwapChainForHwnd(context.queue.Get(), hwnd, &description, nullptr, nullptr, &swapchain1), "CreateSwapChainForHwnd");
        _Check(swapchain1.As(&swapchain->swapchain), "IDXGISwapChain3");

        /* the window decides about full screen itself */
        context.factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

        for (auto i = UINT(0); i < _SWAPCHAIN_BUFFERS; i++) {
            swapchain->rtvs[i] = _AllocateSlot(context.freeRtvs, context.nextRtv, _RTV_CAPACITY);
        }

        swapchain->width = width;
        swapchain->height = height;

        _AcquireSwapchainBuffers(*swapchain);

        it = context.swapchains.emplace(hwnd, swapchain).first;
    }

    auto& swapchain = *it->second;

    /* resized only while no pass recorded on it waits to be submitted: those
       hold its buffers, and a window painted twice in one frame keeps its size */
    if (((swapchain.width != width) || (swapchain.height != height)) && (swapchain.references == 0)) {
        _ReleaseSwapchainBuffers(swapchain);
        _Check(swapchain.swapchain->ResizeBuffers(_SWAPCHAIN_BUFFERS, width, height, _TEXTURE_FORMAT, 0), "IDXGISwapChain::ResizeBuffers");
        _AcquireSwapchainBuffers(swapchain);

        swapchain.width = width;
        swapchain.height = height;
    }

    return &swapchain;
}

void __ReleaseWindowTarget(void* handle) {
    PROFILE

    auto& context = _GetContext();
    auto const it = context.swapchains.find(static_cast<HWND>(handle));

    if (it == context.swapchains.end()) {
        return;
    }

    auto const swapchain = it->second;

    _ReleaseSwapchainBuffers(*swapchain);

    for (auto i = UINT(0); i < _SWAPCHAIN_BUFFERS; i++) {
        context.freeRtvs.push_back(swapchain->rtvs[i]);
    }

    context.swapchains.erase(it);
    delete swapchain;
}

_FilterTextures& Painter::_Painter::getFilterTextures(std::uint32_t width, std::uint32_t height) {
    PROFILE

    assert(width > 0);
    assert(height > 0);

    auto const key = ((static_cast<std::uint64_t>(width) << 32) | static_cast<std::uint64_t>(height));

    auto it = filterTextures.find(key);
    if (it == filterTextures.end()) {
        auto first = static_cast<_Texture*>(__CreateTexture(width, height));
        auto second = static_cast<_Texture*>(__CreateTexture(width, height));

        it = filterTextures.emplace(key, _FilterTextures{
            0, width, height, { first, second },
        }).first;
    }

    it->second.lastUse = ++filterTexturesUse;

    return it->second;
}

void Painter::_Painter::evictFilterTextures() {
    PROFILE

    while (filterTextures.size() > static_cast<std::size_t>(_FILTER_TEXTURE_CACHE_MAX)) {
        auto evict = filterTextures.end();
        for (auto entry = filterTextures.begin(); entry != filterTextures.end(); ++entry) {
            if ((evict == filterTextures.end()) || (entry->second.lastUse < evict->second.lastUse)) {
                evict = entry;
            }
        }

        __ReleaseTexture(std::get<0>(evict->second.textures));
        __ReleaseTexture(std::get<1>(evict->second.textures));

        filterTextures.erase(evict);
    }
}

void Painter::__init() {
    PROFILE

    _impl = new _Painter{};
}

void Painter::__done() {
    PROFILE

    if (_impl == nullptr) {
        return;
    }

    /* The open passes are abandoned: their command list is never run, so
       every target keeps its previous content, a window's as well. */
    if (_impl->recording.open) {
        _DiscardRecording(_impl->recording, _impl->allocators);

        for (auto const swapchain : _impl->presents) {
            swapchain->references--;
        }

        _RetireWaiting();
    }

    for (auto& [ key, filterTextures ] : _impl->filterTextures) {
        __ReleaseTexture(std::get<0>(filterTextures.textures));
        __ReleaseTexture(std::get<1>(filterTextures.textures));
    }

    /* the allocators may still be in flight: they go once the GPU is done */
    _Defer([allocators = std::move(_impl->allocators), list = std::move(_impl->recording.list)]() {});

    delete _impl;
    _impl = nullptr;
}

void Painter::__beginPaint(PaintTarget const& target) {
    PROFILE

    if (_impl->renderPassStack.empty() == false) {
        if (_impl->renderPassStack.top().has_value()) {
            _SuspendRenderPass(*_impl->renderPassStack.top());
        }
    }

    /* the outermost pass opens the command list every nested pass shares */
    auto const openRecording = [&]() {
        if (_impl->recording.open) {
            return false;
        }

        _OpenRecording(_impl->recording, _impl->allocators);

        return true;
    };

    target.match(
        [&](ImagePaintTarget const& imageTarget) {
            assert(imageTarget.image.getTexture() != nullptr);

            auto const commit = openRecording();

            _impl->renderPassStack.emplace(_RenderPass{});
            _impl->renderPassStack.top()->commit = commit;

            _BeginRenderPass(
                *_impl->renderPassStack.top(),
                _impl->recording,
                static_cast<_Texture const*>(imageTarget.image.getTexture()),
                imageTarget.clearColor
            );
        },
        [&](WindowPaintTarget const& windowTarget) {
            assert(windowTarget.window.getHandle() != nullptr);

            auto const swapchain = _GetSwapchain(static_cast<HWND>(windowTarget.window.getHandle()));

            if (swapchain == nullptr) {
                /* nothing to present to (a minimized window): the pass is a no-op */
                _impl->renderPassStack.emplace();
                return;
            }

            auto const commit = openRecording();
            auto const index = swapchain->swapchain->GetCurrentBackBufferIndex();

            /* the buffer was last presented from an earlier frame: wait until the GPU let it go */
            _WaitForFence(swapchain->fenceValues[index]);

            if (std::ranges::find(_impl->presents, swapchain) == _impl->presents.end()) {
                _impl->presents.push_back(swapchain);
                swapchain->references++;
            }

            _impl->renderPassStack.emplace(_RenderPass{});

            auto& renderPass = *_impl->renderPassStack.top();

            renderPass.commit = commit;
            renderPass.width = swapchain->width;
            renderPass.height = swapchain->height;
            renderPass.format = _TEXTURE_FORMAT;
            renderPass.texture = swapchain->buffers[index].Get();
            renderPass.rtv = _GetRtv(swapchain->rtvs[index]);
            renderPass.restState = D3D12_RESOURCE_STATE_PRESENT;
            renderPass.recording = &_impl->recording;

            _OpenRenderPass(renderPass, windowTarget.clearColor);
        }
    );
}

void Painter::__endPaint() {
    PROFILE

    if (_impl->renderPassStack.empty() == false) {
        if (_impl->renderPassStack.top().has_value()) {
            auto& renderPass = *_impl->renderPassStack.top();
            auto const commit = renderPass.commit;

            _EndRenderPass(renderPass);

            if (commit) {
                /* what was uploaded before runs before what was painted */
                _FlushTransfer();

                auto const fence = _SubmitRecording(_impl->recording, _impl->allocators);
                auto const presents = std::move(_impl->presents);

                _impl->presents.clear();

                /* the last window waits for its vertical blank; the others
                   would make every frame wait once per window */
                for (auto i = std::size_t(0); i < presents.size(); i++) {
                    auto const swapchain = presents[i];
                    auto const last = ((i + 1) == presents.size());

                    swapchain->fenceValues[swapchain->swapchain->GetCurrentBackBufferIndex()] = fence;
                    swapchain->references--;

                    /* DXGI_STATUS_OCCLUDED (a hidden window) is not an error */
                    swapchain->swapchain->Present((last ? 1 : 0), 0);
                }

                _RetireWaiting();
                _CollectDeferred();
            }
        }

        _impl->renderPassStack.pop();
    }

    if (_impl->renderPassStack.empty() == false) {
        if (_impl->renderPassStack.top().has_value()) {
            _ResumeRenderPass(*_impl->renderPassStack.top());
        }
    }

    if (_impl->renderPassStack.empty()) {
        _impl->evictFilterTextures();
    }
}

void Painter::__paint(Shape const& shape, Brush const& brush, PaintOptions const& options, Vec4 const& bounds) const {
    PROFILE

    if (_impl->renderPassStack.empty()) {
        return;
    }

    if (_impl->renderPassStack.top().has_value() == false) {
        return;
    }

    assert(_impl->renderPassStack.top()->open);

    auto& renderPass = *_impl->renderPassStack.top();

    auto const colorBrush = brush.as<ColorBrush>();
    auto const imageBrush = brush.as<ImageBrush>();
    auto const gradientBrush = brush.as<GradientBrush>();
    auto const shadowFilter = options.filter.has_value() ? options.filter->as<ShadowFilter>() : nullptr;
    auto const blurFilter = options.filter.has_value() ? options.filter->as<BlurFilter>() : nullptr;
    auto const blend = options.blend.value_or(Blend::Over);

    auto const drawShape = [&](_RenderPass& pass, std::optional<Vec4> const& scissor, Blend mode, Vec2 const& scale) {
        assert(pass.open);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        auto const transform = options.transform.value_or(Mat3{}).toScaled(scale);

        if (colorBrush != nullptr) {
            _SetPipeline(pass, _GetPipeline(_Fragment::ColorBrush, pass.format, mode));
            _PushColorBrushUniforms(pass, *colorBrush);
        } else if (imageBrush != nullptr) {
            _SetPipeline(pass, _GetPipeline(_Fragment::ImageBrush, pass.format, mode));
            _PushImageBrushUniforms(pass, *imageBrush, bounds);
        } else if (gradientBrush != nullptr) {
            if (gradientBrush->radial.value_or(false)) {
                _SetPipeline(pass, _GetPipeline(_Fragment::RadialGradientBrush, pass.format, mode));
            } else {
                _SetPipeline(pass, _GetPipeline(_Fragment::LinearGradientBrush, pass.format, mode));
            }
            _PushGradientBrushUniforms(pass, *gradientBrush);
        }

        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, transform, options.opacity.value_or(1.0f));
        _PushShapeUniforms(pass, shape, _EdgeExpansion(transform), true);

        _Draw(pass);
    };

    auto const blurred = (blurFilter != nullptr) && (blurFilter->radius > 0.0f);

    if (
        blurred == false &&
        shadowFilter == nullptr
    ) {
        drawShape(renderPass, options.scissor, blend, Vec2{ 1.0f, 1.0f });
        return;
    }

    /* The filter runs its passes on a copy downscaled by this factor, and the
       last pass samples that copy bilinearly back up onto the target. The
       shape is drawn straight into the small copy through a scaled transform,
       so its coverage is computed at that size rather than resampled. */
    auto const downsample = _FilterDownsample(*options.filter);
    auto const filterWidth = static_cast<std::uint32_t>(std::ceil(static_cast<float>(renderPass.width) / downsample));
    auto const filterHeight = static_cast<std::uint32_t>(std::ceil(static_cast<float>(renderPass.height) / downsample));
    auto const filterScale = Vec2{
        (static_cast<float>(filterWidth) / static_cast<float>(renderPass.width)),
        (static_cast<float>(filterHeight) / static_cast<float>(renderPass.height))
    };
    auto const filterTexel = Vec2{
        (1.0f / static_cast<float>(filterWidth)),
        (1.0f / static_cast<float>(filterHeight))
    };

    auto const drawBlur = [&](_RenderPass& pass, _Texture const* source, Vec2 const& direction, Vec4 const& rect, std::optional<Vec4> const& scissor, Blend mode) {
        assert(pass.open);
        assert(source != nullptr);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        _SetPipeline(pass, _GetPipeline(_Fragment::BlurFilter, pass.format, mode));
        _BindTexture(pass, source, _LinLinSampler);
        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, Mat3{}, 1.0f);
        _PushShapeUniforms(pass, Shape{ QuadShape{ rect } }, Vec2{}, false);
        _PushBlurFilterUniforms(pass, filterTexel, direction, *blurFilter, filterScale);

        _Draw(pass);
    };

    auto const drawShadowField = [&](_RenderPass& pass, _Texture const* source, _Fragment fragment, Vec2 const& direction, float spread, bool fromSilhouette, Vec4 const& rect) {
        assert(pass.open);
        assert(source != nullptr);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        _SetPipeline(pass, _GetPipeline(fragment, pass.format, Blend::Over));
        _BindTexture(pass, source, _LinLinSampler);
        _SetScissor(pass, rect);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, Mat3{}, 1.0f);
        _PushShapeUniforms(pass, Shape{ QuadShape{ rect } }, Vec2{}, false);
        _PushShadowFilterFieldUniforms(pass, filterTexel, direction, spread, *shadowFilter, fromSilhouette, filterScale);

        _Draw(pass);
    };

    auto const drawShadowComposite = [&](_RenderPass& pass, _Texture const* source, bool fromSilhouette, Vec4 const& rect, std::optional<Vec4> const& scissor, Blend mode) {
        assert(pass.open);
        assert(source != nullptr);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        /* A drop shadow is translated by the offset while sampling at the
           untranslated position, so its quad is the rect shifted back, kept on
           the target the way a full-target quad is. */
        auto const offset = shadowFilter->offset.value_or(Vec2{});
        auto const transform = shadowFilter->inset ? Mat3{} : Mat3{}.toTranslated(offset);
        auto const quad = shadowFilter->inset ? rect : rect.toTranslated(offset * -1.0f).getIntersection(Vec4{ {}, resolution });

        _SetPipeline(pass, _GetPipeline(_Fragment::ShadowFilterComposite, pass.format, mode));
        _BindTexture(pass, source, _LinLinSampler);
        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, transform, 1.0f);
        _PushShapeUniforms(pass, Shape{ QuadShape{ quad } }, Vec2{}, false);
        _PushShadowFilterUniforms(pass, filterTexel, *shadowFilter, fromSilhouette, filterScale);

        _Draw(pass);
    };

    if (blurred) {
        /* The passes only cover what the blur can touch: the shape grown by the
           kernel reach, plus three scratch texels for the tap rounding, the
           bilinear fetch of the last pass and the shape's antialiased edge. */
        auto const grow = (blurFilter->getExtent() + (3.0f * downsample));
        auto const margin = Vec2{ grow, grow };
        auto const filterBounds = _FilterBounds(bounds, options.transform.value_or(Mat3{}), margin, Vec2{ static_cast<float>(renderPass.width), static_cast<float>(renderPass.height) });
        if (filterBounds.has_value() == false) {
            return;
        }

        auto const targetScissor = options.scissor.has_value() ? filterBounds->getIntersection(*options.scissor) : *filterBounds;
        if (targetScissor.isEmpty()) {
            return;
        }

        auto const scratchRect = _ToFilterSpace(*filterBounds, filterScale);

        _SuspendRenderPass(renderPass);

        auto const& [ shapeTexture, blurTexture ] = _impl->getFilterTextures(filterWidth, filterHeight).textures;
        auto const clearColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

        auto shapePass = _RenderPass{};
        _BeginRenderPass(shapePass, _impl->recording, shapeTexture, clearColor);
        drawShape(shapePass, std::nullopt, Blend::Over, filterScale);
        _EndRenderPass(shapePass);

        auto blurPass = _RenderPass{};
        _BeginRenderPass(blurPass, _impl->recording, blurTexture, clearColor);
        drawBlur(blurPass, shapeTexture, Vec2{ 1.0f, 0.0f }, scratchRect, scratchRect, Blend::Over);
        _EndRenderPass(blurPass);

        _ResumeRenderPass(renderPass);
        drawBlur(renderPass, blurTexture, Vec2{ 0.0f, 1.0f }, *filterBounds, targetScissor, blend);
    } else {
        /* As for the blur, grown by the spread and the offset as well. */
        auto const shadowSpread = shadowFilter->spread.value_or(Vec2{});
        auto const shadowOffset = shadowFilter->offset.value_or(Vec2{});
        auto const margin = Vec2{
            (shadowFilter->getExtent() + std::abs(shadowSpread.x) + std::abs(shadowOffset.x) + (3.0f * downsample)),
            (shadowFilter->getExtent() + std::abs(shadowSpread.y) + std::abs(shadowOffset.y) + (3.0f * downsample))
        };
        auto const filterBounds = _FilterBounds(bounds, options.transform.value_or(Mat3{}), margin, Vec2{ static_cast<float>(renderPass.width), static_cast<float>(renderPass.height) });
        if (filterBounds.has_value() == false) {
            return;
        }

        auto const targetScissor = options.scissor.has_value() ? filterBounds->getIntersection(*options.scissor) : *filterBounds;
        if (targetScissor.isEmpty()) {
            return;
        }

        auto const scratchRect = _ToFilterSpace(*filterBounds, filterScale);

        _SuspendRenderPass(renderPass);

        auto [ source, target ] = _impl->getFilterTextures(filterWidth, filterHeight).textures;
        auto const clearColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
        auto const spread = shadowFilter->spread.value_or(Vec2{});
        auto fromSilhouette = true;

        auto shapePass = _RenderPass{};
        _BeginRenderPass(shapePass, _impl->recording, source, clearColor);
        drawShape(shapePass, std::nullopt, Blend::Over, filterScale);
        _EndRenderPass(shapePass);

        auto const fieldPass = [&](_Fragment fragment, Vec2 const& direction, float extent) {
            auto pass = _RenderPass{};
            _BeginRenderPass(pass, _impl->recording, target, clearColor);
            drawShadowField(pass, source, fragment, direction, extent, fromSilhouette, scratchRect);
            _EndRenderPass(pass);
            std::swap(source, target);
            fromSilhouette = false;
        };

        if (spread.x != 0.0f) {
            fieldPass(_Fragment::ShadowFilterDilate, Vec2{ 1.0f, 0.0f }, spread.x);
        }
        if (spread.y != 0.0f) {
            fieldPass(_Fragment::ShadowFilterDilate, Vec2{ 0.0f, 1.0f }, spread.y);
        }
        if (shadowFilter->getExtent() > 0.0f) {
            fieldPass(_Fragment::ShadowFilterBlur, Vec2{ 1.0f, 0.0f }, 0.0f);
        }

        _ResumeRenderPass(renderPass);
        drawShadowComposite(renderPass, source, fromSilhouette, *filterBounds, targetScissor, blend);
    }
}

} /* namespace Rocket */
