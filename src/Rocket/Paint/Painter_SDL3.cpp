/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <map>
#include <cmath>
#include <stack>
#include <tuple>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <algorithm>
#include <stdexcept>
#include <SDL3/SDL.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Painter.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Painter_Private.hpp>
#include <Rocket/Window/Window.hpp>
#include <Rocket/Window/Window_SDL3.hpp>
#include "QuadVertex.spv.hpp"
#include "ColorBrush.spv.hpp"
#include "ImageBrush.spv.hpp"
#include "LinearGradientBrush.spv.hpp"
#include "RadialGradientBrush.spv.hpp"
#include "BlurFilter.spv.hpp"
#include "ShadowFilterDilate.spv.hpp"
#include "ShadowFilterBlur.spv.hpp"
#include "ShadowFilterComposite.spv.hpp"

namespace Rocket {

/* The uniform blocks of SDL3/Painter.glsl in std140 layout: vec2 aligns to 8
   bytes, vec4, mat3 columns and array elements to 16, so the padding is spelled
   out and the offsets are checked below. */

/** Per-pass state shared by every draw: resolution, projection, transform, and opacity. */
struct _StateUniforms {
    float resolution[2];
    float _pad0[2];
    float projection[12]; // three columns, each padded to four floats
    float transform[12];  // three columns, each padded to four floats
    float opacity;
    float _pad1[3];
};

/** Geometry of the shape being drawn, shared by the vertex and fragment stages. */
struct _ShapeUniforms {
    float vertices[4][4]; // .xy used; vec4 for the std140 array stride
    float bounds[4];
    float borders[4]; // per-edge outline widths (left, top, right, bottom)
    float corners[4]; // per-corner radii (top-left, top-right, bottom-right, bottom-left)
    std::int32_t type; // 0=quad, 1=ellipse, 2=quad outline, 3=ellipse outline
    std::int32_t _pad0[3];
};

/** Uniforms for the ColorBrush fragment shader. */
struct _ColorBrushUniforms {
    float color[4];
};

/** Uniforms for the ImageBrush fragment shader. */
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

/** Uniforms for the BlurFilter two-pass gaussian blur fragment shader. */
struct _BlurFilterUniforms {
    float texelSize[2];
    float direction[2];
    float sigma;
    std::int32_t taps;
    std::int32_t _pad0[2];
};

/** Uniforms for the ShadowFilter composite pass. */
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

/** Uniforms for the ShadowFilter separable dilate and blur passes. */
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

/** The fragment shaders, one SPIR-V module each. */
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

struct _RenderPass {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUCommandBuffer* commandBuffer = nullptr;
    SDL_GPURenderPass* pass = nullptr;
    bool commit = false;
};

struct _FilterTextures {
    std::uint64_t lastUse = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::tuple<
        SDL_GPUTexture*,
        SDL_GPUTexture*
    > textures;
};

struct Painter::_Painter {
    SDL_GPUCommandBuffer* commandBuffer = nullptr;
    bool swapchainAcquired = false; // the command buffer holds a window's swapchain texture: it can only be submitted
    std::stack<std::optional<_RenderPass>> renderPassStack;
    std::map<std::uint64_t, _FilterTextures> filterTextures; // keyed by (width << 32) | height
    std::uint64_t filterTexturesUse = 0;
    _FilterTextures& getFilterTextures(std::uint32_t width, std::uint32_t height);
    void evictFilterTextures();
};

static auto constexpr _BLUR_TAP_MAX = 64;
static auto constexpr _FILTER_TEXTURE_CACHE_MAX = 8;
static auto constexpr _STATE_VERTEX_UNIFORMS_INDEX   = 0;
static auto constexpr _SHAPE_VERTEX_UNIFORMS_INDEX   = 1;
static auto constexpr _STATE_FRAGMENT_UNIFORMS_INDEX = 0;
static auto constexpr _BRUSH_FRAGMENT_UNIFORMS_INDEX = 1;
static auto constexpr _SHAPE_FRAGMENT_UNIFORMS_INDEX = 2;

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

static SDL_GPUColorTargetBlendState _GetBlend(Blend blend) {
    PROFILE

    auto state = SDL_GPUColorTargetBlendState{};
    state.enable_blend = true;
    state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    switch (blend) {
        case Blend::Over:
            // dst = src + dst * (1 - srcA)
            state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            break;
        case Blend::Add:
            // dst = src + dst; the target's alpha is kept
            state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
            state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            break;
        case Blend::Multiply:
            // dst = src * dst + dst * (1 - srcA): where the source is transparent the target stays
            state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR;
            state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
            state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            break;
        case Blend::Screen:
            // dst = src + dst * (1 - src)
            state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
            state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
            state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            break;
    }

    return state;
}

static void _SetScissor(_RenderPass const& renderPass, std::optional<Vec4> const& scissor) {
    PROFILE

    assert(renderPass.pass != nullptr);

    auto const passWidth = static_cast<int>(renderPass.width);
    auto const passHeight = static_cast<int>(renderPass.height);

    auto scissorInfo = SDL_Rect{};

    if (scissor.has_value()) {
        auto const left = std::max(0, static_cast<int>(std::roundf(scissor->x)));
        auto const top = std::max(0, static_cast<int>(std::roundf(scissor->y)));
        auto const right = std::min(passWidth, static_cast<int>(std::roundf(scissor->x + scissor->width)));
        auto const bottom = std::min(passHeight, static_cast<int>(std::roundf(scissor->y + scissor->height)));

        scissorInfo = SDL_Rect{
            std::min(left, passWidth),
            std::min(top, passHeight),
            std::max(0, right - left),
            std::max(0, bottom - top)
        };
    } else {
        scissorInfo = SDL_Rect{
            0, 0,
            passWidth,
            passHeight
        };
    }

    ::SDL_SetGPUScissor(renderPass.pass, &scissorInfo);
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

/* a target-space rect in the downscaled scratch copy, snapped outward to whole texels */
static Vec4 _ToFilterSpace(Vec4 const& rect, Vec2 const& scale) {
    PROFILE

    auto const left   = std::floor(rect.x * scale.x);
    auto const top    = std::floor(rect.y * scale.y);
    auto const right  = std::ceil(rect.getMaxX() * scale.x);
    auto const bottom = std::ceil(rect.getMaxY() * scale.y);

    return Vec4{ left, top, (right - left), (bottom - top) };
}

static void _SetViewport(_RenderPass const& renderPass, Vec4 const& viewport) {
    PROFILE

    assert(renderPass.pass != nullptr);

    auto const viewportInfo = SDL_GPUViewport{
        viewport.x,
        viewport.y,
        viewport.width,
        viewport.height,
        0.0f,
        1.0f
    };

    ::SDL_SetGPUViewport(renderPass.pass, &viewportInfo);
}

static SDL_GPUDevice* _GetDevice() {
    PROFILE

    return static_cast<SDL_GPUDevice*>(__GetDefaultGPUDevice());
}

static SDL_GPUShader* _CreateShader(std::uint8_t const* code, std::size_t size, SDL_GPUShaderStage stage, std::uint32_t samplers, std::uint32_t uniformBuffers) {
    PROFILE

    auto info = SDL_GPUShaderCreateInfo{};
    info.code_size = size;
    info.code = code;
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage = stage;
    info.num_samplers = samplers;
    info.num_uniform_buffers = uniformBuffers;

    auto shader = ::SDL_CreateGPUShader(_GetDevice(), &info);

    if (shader == nullptr) {
        throw std::runtime_error(::SDL_GetError());
    }

    return shader;
}

static SDL_GPUShader* _GetVertexShader() {
    PROFILE

    static auto const _shader = _CreateShader(Rocket_SPIRV_QuadVertex, Rocket_SPIRV_QuadVertex_len, SDL_GPU_SHADERSTAGE_VERTEX, 0, 2);

    return _shader;
}

static SDL_GPUShader* _GetFragmentShader(_Fragment fragment) {
    PROFILE

    static auto _shaders = std::map<_Fragment, SDL_GPUShader*>();

    auto it = _shaders.find(fragment);
    if (it != _shaders.end()) {
        return it->second;
    }

    /* uniform buffer counts run to the highest slot a shader declares: the
       filter shaders take their uniforms at the brush slot, as on Metal */
    auto shader = static_cast<SDL_GPUShader*>(nullptr);
    auto constexpr stage = SDL_GPU_SHADERSTAGE_FRAGMENT;

    switch (fragment) {
        case _Fragment::ColorBrush:            shader = _CreateShader(Rocket_SPIRV_ColorBrush, Rocket_SPIRV_ColorBrush_len, stage, 0, 3); break;
        case _Fragment::ImageBrush:            shader = _CreateShader(Rocket_SPIRV_ImageBrush, Rocket_SPIRV_ImageBrush_len, stage, 1, 3); break;
        case _Fragment::LinearGradientBrush:   shader = _CreateShader(Rocket_SPIRV_LinearGradientBrush, Rocket_SPIRV_LinearGradientBrush_len, stage, 0, 3); break;
        case _Fragment::RadialGradientBrush:   shader = _CreateShader(Rocket_SPIRV_RadialGradientBrush, Rocket_SPIRV_RadialGradientBrush_len, stage, 0, 3); break;
        case _Fragment::BlurFilter:            shader = _CreateShader(Rocket_SPIRV_BlurFilter, Rocket_SPIRV_BlurFilter_len, stage, 1, 2); break;
        case _Fragment::ShadowFilterDilate:    shader = _CreateShader(Rocket_SPIRV_ShadowFilterDilate, Rocket_SPIRV_ShadowFilterDilate_len, stage, 1, 2); break;
        case _Fragment::ShadowFilterBlur:      shader = _CreateShader(Rocket_SPIRV_ShadowFilterBlur, Rocket_SPIRV_ShadowFilterBlur_len, stage, 1, 2); break;
        case _Fragment::ShadowFilterComposite: shader = _CreateShader(Rocket_SPIRV_ShadowFilterComposite, Rocket_SPIRV_ShadowFilterComposite_len, stage, 1, 2); break;
    }

    _shaders.emplace(fragment, shader);

    return shader;
}

static SDL_GPUSampler* _CreateSampler(SDL_GPUFilter minFilter, SDL_GPUFilter magFilter) {
    PROFILE

    auto info = SDL_GPUSamplerCreateInfo{};
    info.min_filter = minFilter;
    info.mag_filter = magFilter;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    auto sampler = ::SDL_CreateGPUSampler(_GetDevice(), &info);

    if (sampler == nullptr) {
        throw std::runtime_error(::SDL_GetError());
    }

    return sampler;
}

static SDL_GPUGraphicsPipeline* _CreatePipeline(_Fragment fragment, SDL_GPUTextureFormat format, Blend blend) {
    PROFILE

    auto colorTarget = SDL_GPUColorTargetDescription{};
    colorTarget.format = format;
    colorTarget.blend_state = _GetBlend(blend);

    auto info = SDL_GPUGraphicsPipelineCreateInfo{};
    info.vertex_shader = _GetVertexShader();
    info.fragment_shader = _GetFragmentShader(fragment);
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    info.target_info.color_target_descriptions = &colorTarget;
    info.target_info.num_color_targets = 1;

    auto pipeline = ::SDL_CreateGPUGraphicsPipeline(_GetDevice(), &info);

    if (pipeline == nullptr) {
        throw std::runtime_error(::SDL_GetError());
    }

    return pipeline;
}

static SDL_GPUGraphicsPipeline* _GetPipeline(_Fragment fragment, SDL_GPUTextureFormat format, Blend blend) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, SDL_GPUGraphicsPipeline*>();

    auto const key = (((std::uint64_t)fragment) << 40) | (((std::uint64_t)format) << 8) | (std::uint64_t)blend;
    auto it = _pipelines.find(key);
    if (it == _pipelines.end()) {
        it = _pipelines.emplace(key, _CreatePipeline(fragment, format, blend)).first;
    }

    return it->second;
}

static SDL_GPUSampler* _GetLinLinSampler() {
    PROFILE

    static auto const _sampler = _CreateSampler(SDL_GPU_FILTER_LINEAR, SDL_GPU_FILTER_LINEAR);

    return _sampler;
}

static SDL_GPUSampler* _GetNerNerSampler() {
    PROFILE

    static auto const _sampler = _CreateSampler(SDL_GPU_FILTER_NEAREST, SDL_GPU_FILTER_NEAREST);

    return _sampler;
}

static SDL_GPUSampler* _GetLinNerSampler() {
    PROFILE

    static auto const _sampler = _CreateSampler(SDL_GPU_FILTER_LINEAR, SDL_GPU_FILTER_NEAREST);

    return _sampler;
}

static SDL_GPUSampler* _GetNerLinSampler() {
    PROFILE

    static auto const _sampler = _CreateSampler(SDL_GPU_FILTER_NEAREST, SDL_GPU_FILTER_LINEAR);

    return _sampler;
}

static void _BindTexture(_RenderPass const& renderPass, SDL_GPUTexture* texture, SDL_GPUSampler* sampler) {
    PROFILE

    assert(renderPass.pass != nullptr);
    assert(texture != nullptr);

    auto const binding = SDL_GPUTextureSamplerBinding{ texture, sampler };

    ::SDL_BindGPUFragmentSamplers(renderPass.pass, 0, &binding, 1);
}

static int _GetGaussianTaps(float extent) {
    return std::clamp(static_cast<int>(std::ceil(extent)), 0, _BLUR_TAP_MAX);
}

static void _OpenRenderPass(_RenderPass& renderPass, std::optional<Vec4> const& clear) {
    PROFILE

    auto target = SDL_GPUColorTargetInfo{};
    target.texture = renderPass.texture;
    target.load_op = SDL_GPU_LOADOP_LOAD;
    target.store_op = SDL_GPU_STOREOP_STORE;
    target.cycle = false;

    if (clear.has_value()) {
        target.load_op = SDL_GPU_LOADOP_CLEAR;
        target.clear_color = SDL_FColor{ /* the target is premultiplied; the clear color is straight RGBA */
            (clear->red * clear->alpha),
            (clear->green * clear->alpha),
            (clear->blue * clear->alpha),
            clear->alpha
        };
    }

    renderPass.pass = ::SDL_BeginGPURenderPass(renderPass.commandBuffer, &target, 1, nullptr);

    if (renderPass.pass == nullptr) {
        throw std::runtime_error(::SDL_GetError());
    }
}

static void _BeginRenderPass(_RenderPass& renderPass, SDL_GPUCommandBuffer* command, SDL_GPUTexture* texture, std::uint32_t width, std::uint32_t height, SDL_GPUTextureFormat format, std::optional<Vec4> const& clear) {
    PROFILE

    assert(command != nullptr);
    assert(texture != nullptr);
    assert(width > 0);
    assert(height > 0);

    assert(renderPass.commandBuffer == nullptr);
    assert(renderPass.pass == nullptr);
    assert(renderPass.texture == nullptr);

    renderPass.width = width;
    renderPass.height = height;
    renderPass.format = format;
    renderPass.texture = texture;
    renderPass.commandBuffer = command;

    _OpenRenderPass(renderPass, clear);
}

static void _EndRenderPass(_RenderPass& renderPass) {
    PROFILE

    assert(renderPass.commandBuffer != nullptr);
    assert(renderPass.pass != nullptr);
    assert(renderPass.texture != nullptr);

    ::SDL_EndGPURenderPass(renderPass.pass);

    renderPass.width = 0;
    renderPass.height = 0;
    renderPass.texture = nullptr;
    renderPass.pass = nullptr;
    renderPass.commandBuffer = nullptr;
    renderPass.format = SDL_GPU_TEXTUREFORMAT_INVALID;
    renderPass.commit = false;
}

static void _SuspendRenderPass(_RenderPass& renderPass) {
    PROFILE

    assert(renderPass.commandBuffer != nullptr);
    assert(renderPass.pass != nullptr);
    assert(renderPass.texture != nullptr);

    ::SDL_EndGPURenderPass(renderPass.pass);

    renderPass.pass = nullptr;
}

static void _ResumeRenderPass(_RenderPass& renderPass) {
    PROFILE

    assert(renderPass.commandBuffer != nullptr);
    assert(renderPass.texture != nullptr);

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

    assert(renderPass.pass != nullptr);

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

    ::SDL_PushGPUVertexUniformData(renderPass.commandBuffer, _STATE_VERTEX_UNIFORMS_INDEX, &stateUniforms, sizeof(_StateUniforms));
    ::SDL_PushGPUFragmentUniformData(renderPass.commandBuffer, _STATE_FRAGMENT_UNIFORMS_INDEX, &stateUniforms, sizeof(_StateUniforms));
}

static void _PushShapeUniforms(_RenderPass const& renderPass, Shape const& shape, Vec2 const& expand, bool fragment) {
    PROFILE

    assert(renderPass.pass != nullptr);

    auto rect    = Vec4{};
    auto borders = Vec4{}; // per-edge outline widths (left, top, right, bottom)
    auto corners = Vec4{}; // per-corner radii (top-left, top-right, bottom-right, bottom-left)
    auto type    = 0;      // 0=quad, 1=ellipse, 2=quad outline, 3=ellipse outline

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

    ::SDL_PushGPUVertexUniformData(renderPass.commandBuffer, _SHAPE_VERTEX_UNIFORMS_INDEX, &shapeUniforms, sizeof(_ShapeUniforms));

    if (fragment) {
        ::SDL_PushGPUFragmentUniformData(renderPass.commandBuffer, _SHAPE_FRAGMENT_UNIFORMS_INDEX, &shapeUniforms, sizeof(_ShapeUniforms));
    }
}

static void _PushColorBrushUniforms(_RenderPass const& renderPass, ColorBrush const& colorBrush) {
    PROFILE

    assert(renderPass.pass != nullptr);

    auto const colorUniforms = _ColorBrushUniforms{
        .color = { colorBrush.color.red, colorBrush.color.green, colorBrush.color.blue, colorBrush.color.alpha }
    };

    ::SDL_PushGPUFragmentUniformData(renderPass.commandBuffer, _BRUSH_FRAGMENT_UNIFORMS_INDEX, &colorUniforms, sizeof(_ColorBrushUniforms));
}

static void _PushImageBrushUniforms(_RenderPass const& renderPass, ImageBrush const& imageBrush, Vec4 const& bounds) {
    PROFILE

    assert(renderPass.pass != nullptr);
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

    auto sampler = _GetNerNerSampler();

    if (filterMin == ImageFilter::Linear && filterMag == ImageFilter::Linear) {
        sampler = _GetLinLinSampler();
    } else if (filterMin == ImageFilter::Linear && filterMag == ImageFilter::Nearest) {
        sampler = _GetLinNerSampler();
    } else if (filterMin == ImageFilter::Nearest && filterMag == ImageFilter::Linear) {
        sampler = _GetNerLinSampler();
    }

    _BindTexture(renderPass, static_cast<SDL_GPUTexture*>(imageBrush.image->getTexture()), sampler);
    ::SDL_PushGPUFragmentUniformData(renderPass.commandBuffer, _BRUSH_FRAGMENT_UNIFORMS_INDEX, &imageUniforms, sizeof(_ImageBrushUniforms));
}

static void _PushGradientBrushUniforms(_RenderPass const& renderPass, GradientBrush const& gradientBrush) {
    PROFILE

    assert(renderPass.pass != nullptr);

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

    ::SDL_PushGPUFragmentUniformData(renderPass.commandBuffer, _BRUSH_FRAGMENT_UNIFORMS_INDEX, &gradientUniforms, sizeof(_GradientBrushUniforms));
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

    ::SDL_PushGPUFragmentUniformData(renderPass.commandBuffer, _BRUSH_FRAGMENT_UNIFORMS_INDEX, &blurUniforms, sizeof(_BlurFilterUniforms));
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

    ::SDL_PushGPUFragmentUniformData(renderPass.commandBuffer, _BRUSH_FRAGMENT_UNIFORMS_INDEX, &fieldUniforms, sizeof(_ShadowFilterFieldUniforms));
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

    ::SDL_PushGPUFragmentUniformData(renderPass.commandBuffer, _BRUSH_FRAGMENT_UNIFORMS_INDEX, &shadowUniforms, sizeof(_ShadowFilterUniforms));
}

void* __GetDefaultGPUDevice() {
    PROFILE

    static auto const _device = [] {
        __InitVideo();

        auto device = ::SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, nullptr);

        if (device == nullptr) {
            throw std::runtime_error(::SDL_GetError());
        }

        return device;
    }();

    return _device;
}

int __GetDefaultTextureFormat() {
    PROFILE

    return (int)SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
}

_FilterTextures& Painter::_Painter::getFilterTextures(std::uint32_t width, std::uint32_t height) {
    PROFILE

    assert(width > 0);
    assert(height > 0);

    auto const key = ((static_cast<std::uint64_t>(width) << 32) | static_cast<std::uint64_t>(height));

    auto it = filterTextures.find(key);
    if (it == filterTextures.end()) {
        auto info = SDL_GPUTextureCreateInfo{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = (SDL_GPUTextureFormat)__GetDefaultTextureFormat();
        info.usage = (SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET);
        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;

        auto first = ::SDL_CreateGPUTexture(_GetDevice(), &info);
        auto second = ::SDL_CreateGPUTexture(_GetDevice(), &info);

        if ((first == nullptr) || (second == nullptr)) {
            throw std::runtime_error(::SDL_GetError());
        }

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

        /* released once the GPU is done with them */
        ::SDL_ReleaseGPUTexture(_GetDevice(), std::get<0>(evict->second.textures));
        ::SDL_ReleaseGPUTexture(_GetDevice(), std::get<1>(evict->second.textures));

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

    while (_impl->renderPassStack.empty() == false) {
        auto& renderPass = _impl->renderPassStack.top();
        if (renderPass.has_value() && (renderPass->pass != nullptr)) {
            _EndRenderPass(*renderPass);
        }
        _impl->renderPassStack.pop();
    }

    /* The open passes are abandoned: their command buffer is cancelled, so
       every target keeps its previous content. SDL_GPU cannot cancel a command
       buffer that holds a window's swapchain texture, so that one is submitted
       instead, and the window shows what was painted so far. */
    if (_impl->commandBuffer != nullptr) {
        if (_impl->swapchainAcquired) {
            ::SDL_SubmitGPUCommandBuffer(_impl->commandBuffer);
        } else {
            ::SDL_CancelGPUCommandBuffer(_impl->commandBuffer);
        }
    }

    for (auto& [ key, filterTextures ] : _impl->filterTextures) {
        ::SDL_ReleaseGPUTexture(_GetDevice(), std::get<0>(filterTextures.textures));
        ::SDL_ReleaseGPUTexture(_GetDevice(), std::get<1>(filterTextures.textures));
    }

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

    /* the outermost pass acquires the command buffer every nested pass shares */
    auto const acquireCommandBuffer = [&]() {
        if (_impl->commandBuffer != nullptr) {
            return false;
        }

        _impl->commandBuffer = ::SDL_AcquireGPUCommandBuffer(_GetDevice());

        if (_impl->commandBuffer == nullptr) {
            throw std::runtime_error(::SDL_GetError());
        }

        return true;
    };

    target.match(
        [&](ImagePaintTarget const& imageTarget) {
            assert(imageTarget.image.getTexture() != nullptr);

            _impl->renderPassStack.emplace(_RenderPass{});
            _impl->renderPassStack.top()->commit = acquireCommandBuffer();

            _BeginRenderPass(
                *_impl->renderPassStack.top(),
                _impl->commandBuffer,
                static_cast<SDL_GPUTexture*>(imageTarget.image.getTexture()),
                (std::uint32_t)imageTarget.image.getSize().width,
                (std::uint32_t)imageTarget.image.getSize().height,
                (SDL_GPUTextureFormat)__GetDefaultTextureFormat(),
                imageTarget.clearColor
            );
        },
        [&](WindowPaintTarget const& windowTarget) {
            assert(windowTarget.window.getHandle() != nullptr);

            auto window = static_cast<SDL_Window*>(windowTarget.window.getHandle());
            auto const commit = acquireCommandBuffer();
            auto texture = static_cast<SDL_GPUTexture*>(nullptr);
            auto width = std::uint32_t{ 0 };
            auto height = std::uint32_t{ 0 };

            if (::SDL_WaitAndAcquireGPUSwapchainTexture(_impl->commandBuffer, window, &texture, &width, &height) == false) {
                texture = nullptr;
            }

            if ((texture != nullptr) && (width > 0) && (height > 0)) {
                _impl->swapchainAcquired = true;
                _impl->renderPassStack.emplace(_RenderPass{});
                _impl->renderPassStack.top()->commit = commit;

                _BeginRenderPass(
                    *_impl->renderPassStack.top(),
                    _impl->commandBuffer,
                    texture,
                    width,
                    height,
                    ::SDL_GetGPUSwapchainTextureFormat(_GetDevice(), window),
                    windowTarget.clearColor
                );
            } else {
                /* no swapchain texture (a minimized window): the pass is a
                   no-op, and a command buffer acquired just for it goes away */
                if (commit && (_impl->swapchainAcquired == false)) {
                    ::SDL_CancelGPUCommandBuffer(_impl->commandBuffer);
                    _impl->commandBuffer = nullptr;
                }

                _impl->renderPassStack.emplace();
            }
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
                ::SDL_SubmitGPUCommandBuffer(_impl->commandBuffer); /* presents the swapchain texture, if any */
                _impl->commandBuffer = nullptr;
                _impl->swapchainAcquired = false;
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

    assert(_impl->renderPassStack.top()->pass != nullptr);

    auto& renderPass = *_impl->renderPassStack.top();

    auto const colorBrush = brush.as<ColorBrush>();
    auto const imageBrush = brush.as<ImageBrush>();
    auto const gradientBrush = brush.as<GradientBrush>();
    auto const shadowFilter = options.filter.has_value() ? options.filter->as<ShadowFilter>() : nullptr;
    auto const blurFilter = options.filter.has_value() ? options.filter->as<BlurFilter>() : nullptr;
    auto const blend = options.blend.value_or(Blend::Over);

    auto const drawShape = [&](_RenderPass& pass, std::optional<Vec4> const& scissor, Blend mode, Vec2 const& scale) {
        assert(pass.pass != nullptr);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        auto const transform = options.transform.value_or(Mat3{}).toScaled(scale);

        if (colorBrush != nullptr) {
            ::SDL_BindGPUGraphicsPipeline(pass.pass, _GetPipeline(_Fragment::ColorBrush, pass.format, mode));
            _PushColorBrushUniforms(pass, *colorBrush);
        } else if (imageBrush != nullptr) {
            ::SDL_BindGPUGraphicsPipeline(pass.pass, _GetPipeline(_Fragment::ImageBrush, pass.format, mode));
            _PushImageBrushUniforms(pass, *imageBrush, bounds);
        } else if (gradientBrush != nullptr) {
            if (gradientBrush->radial.value_or(false)) {
                ::SDL_BindGPUGraphicsPipeline(pass.pass, _GetPipeline(_Fragment::RadialGradientBrush, pass.format, mode));
            } else {
                ::SDL_BindGPUGraphicsPipeline(pass.pass, _GetPipeline(_Fragment::LinearGradientBrush, pass.format, mode));
            }
            _PushGradientBrushUniforms(pass, *gradientBrush);
        }

        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, transform, options.opacity.value_or(1.0f));
        _PushShapeUniforms(pass, shape, _EdgeExpansion(transform), true);

        ::SDL_DrawGPUPrimitives(pass.pass, 4, 1, 0, 0);
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

    auto const drawBlur = [&](_RenderPass& pass, SDL_GPUTexture* source, Vec2 const& direction, Vec4 const& rect, std::optional<Vec4> const& scissor, Blend mode) {
        assert(pass.pass != nullptr);
        assert(source != nullptr);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        ::SDL_BindGPUGraphicsPipeline(pass.pass, _GetPipeline(_Fragment::BlurFilter, pass.format, mode));
        _BindTexture(pass, source, _GetLinLinSampler());
        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, Mat3{}, 1.0f);
        _PushShapeUniforms(pass, Shape{ QuadShape{ rect } }, Vec2{}, false);
        _PushBlurFilterUniforms(pass, filterTexel, direction, *blurFilter, filterScale);

        ::SDL_DrawGPUPrimitives(pass.pass, 4, 1, 0, 0);
    };

    auto const drawShadowField = [&](_RenderPass& pass, SDL_GPUTexture* source, _Fragment fragment, Vec2 const& direction, float spread, bool fromSilhouette, Vec4 const& rect) {
        assert(pass.pass != nullptr);
        assert(source != nullptr);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        ::SDL_BindGPUGraphicsPipeline(pass.pass, _GetPipeline(fragment, pass.format, Blend::Over));
        _BindTexture(pass, source, _GetLinLinSampler());
        _SetScissor(pass, rect);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, Mat3{}, 1.0f);
        _PushShapeUniforms(pass, Shape{ QuadShape{ rect } }, Vec2{}, false);
        _PushShadowFilterFieldUniforms(pass, filterTexel, direction, spread, *shadowFilter, fromSilhouette, filterScale);

        ::SDL_DrawGPUPrimitives(pass.pass, 4, 1, 0, 0);
    };

    auto const drawShadowComposite = [&](_RenderPass& pass, SDL_GPUTexture* source, bool fromSilhouette, Vec4 const& rect, std::optional<Vec4> const& scissor, Blend mode) {
        assert(pass.pass != nullptr);
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

        ::SDL_BindGPUGraphicsPipeline(pass.pass, _GetPipeline(_Fragment::ShadowFilterComposite, pass.format, mode));
        _BindTexture(pass, source, _GetLinLinSampler());
        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, transform, 1.0f);
        _PushShapeUniforms(pass, Shape{ QuadShape{ quad } }, Vec2{}, false);
        _PushShadowFilterUniforms(pass, filterTexel, *shadowFilter, fromSilhouette, filterScale);

        ::SDL_DrawGPUPrimitives(pass.pass, 4, 1, 0, 0);
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
        auto const filterFormat = (SDL_GPUTextureFormat)__GetDefaultTextureFormat();
        auto const clearColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

        auto shapePass = _RenderPass{};
        _BeginRenderPass(shapePass, renderPass.commandBuffer, shapeTexture, filterWidth, filterHeight, filterFormat, clearColor);
        drawShape(shapePass, std::nullopt, Blend::Over, filterScale);
        _EndRenderPass(shapePass);

        auto blurPass = _RenderPass{};
        _BeginRenderPass(blurPass, renderPass.commandBuffer, blurTexture, filterWidth, filterHeight, filterFormat, clearColor);
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
        auto const filterFormat = (SDL_GPUTextureFormat)__GetDefaultTextureFormat();
        auto const clearColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
        auto const spread = shadowFilter->spread.value_or(Vec2{});
        auto fromSilhouette = true;

        auto shapePass = _RenderPass{};
        _BeginRenderPass(shapePass, renderPass.commandBuffer, source, filterWidth, filterHeight, filterFormat, clearColor);
        drawShape(shapePass, std::nullopt, Blend::Over, filterScale);
        _EndRenderPass(shapePass);

        auto const fieldPass = [&](_Fragment fragment, Vec2 const& direction, float extent) {
            auto pass = _RenderPass{};
            _BeginRenderPass(pass, renderPass.commandBuffer, target, filterWidth, filterHeight, filterFormat, clearColor);
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
