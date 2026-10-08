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
#include <cstdint>
#include <cstring>
#include <optional>
#include <algorithm>
#include <stdexcept>
#import <Metal/Metal.h>
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Painter.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Painter_Private.hpp>
#include <Rocket/Window/Window.hpp>
#include "Metal/Painter.metal.hpp"
#include "Painter.metal-lib.hpp"

namespace Rocket {

struct _RenderPass {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    MTLPixelFormat format = MTLPixelFormatInvalid;
    id<MTLTexture> texture = nil;
    id<CAMetalDrawable> drawable = nil;
    id<MTLCommandBuffer> commandBuffer = nil;
    id<MTLRenderCommandEncoder> encoder = nil;
    bool commit = false;
};

struct _FilterTextures {
    std::uint64_t lastUse = 0;
    std::tuple<
        id<MTLTexture>,
        id<MTLTexture>
    > textures;
};

struct Painter::_Painter {
    id<MTLCommandBuffer> commandBuffer = nil;
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

static void _SetBlend(MTLRenderPipelineColorAttachmentDescriptor* attachment, Blend blend) {
    PROFILE

    attachment.blendingEnabled = YES;
    attachment.rgbBlendOperation = MTLBlendOperationAdd;
    attachment.alphaBlendOperation = MTLBlendOperationAdd;

    switch (blend) {
        case Blend::Over:
            // dst = src + dst * (1 - srcA)
            attachment.sourceRGBBlendFactor = MTLBlendFactorOne;
            attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            attachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
            attachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            break;
        case Blend::Add:
            // dst = src + dst; the target's alpha is kept
            attachment.sourceRGBBlendFactor = MTLBlendFactorOne;
            attachment.destinationRGBBlendFactor = MTLBlendFactorOne;
            attachment.sourceAlphaBlendFactor = MTLBlendFactorZero;
            attachment.destinationAlphaBlendFactor = MTLBlendFactorOne;
            break;
        case Blend::Multiply:
            // dst = src * dst + dst * (1 - srcA): where the source is transparent the target stays
            attachment.sourceRGBBlendFactor = MTLBlendFactorDestinationColor;
            attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            attachment.sourceAlphaBlendFactor = MTLBlendFactorZero;
            attachment.destinationAlphaBlendFactor = MTLBlendFactorOne;
            break;
        case Blend::Screen:
            // dst = src + dst * (1 - src)
            attachment.sourceRGBBlendFactor = MTLBlendFactorOne;
            attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceColor;
            attachment.sourceAlphaBlendFactor = MTLBlendFactorZero;
            attachment.destinationAlphaBlendFactor = MTLBlendFactorOne;
            break;
    }
}

static void _SetScissor(_RenderPass const& renderPass, std::optional<Vec4> const& scissor) {
    PROFILE

    assert(renderPass.encoder != nil);

    auto const passWidth = static_cast<int>(renderPass.width);
    auto const passHeight = static_cast<int>(renderPass.height);

    auto scissorInfo = MTLScissorRect{};

    if (scissor.has_value()) {
        auto const left = std::max(0, static_cast<int>(std::roundf(scissor->x)));
        auto const top = std::max(0, static_cast<int>(std::roundf(scissor->y)));
        auto const right = std::min(passWidth, static_cast<int>(std::roundf(scissor->x + scissor->width)));
        auto const bottom = std::min(passHeight, static_cast<int>(std::roundf(scissor->y + scissor->height)));

        scissorInfo = MTLScissorRect{
            (NSUInteger)std::min(left, passWidth),
            (NSUInteger)std::min(top, passHeight),
            (NSUInteger)std::max(0, right - left),
            (NSUInteger)std::max(0, bottom - top)
        };
    } else {
        scissorInfo = MTLScissorRect{
            0, 0,
            (NSUInteger)passWidth,
            (NSUInteger)passHeight
        };
    }

    [renderPass.encoder setScissorRect: scissorInfo];
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

    if ((rect.width <= 0.0f) || (rect.height <= 0.0f)) {
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

    assert(renderPass.encoder != nil);

    [renderPass.encoder setViewport: MTLViewport{
        viewport.x,
        viewport.y,
        viewport.width,
        viewport.height,
        0.0,
        1.0
    }];
}

static id<MTLDevice> _GetDevice() {
    PROFILE

    return (__bridge id<MTLDevice>)__GetDefaultGPUDevice();
}

static id<MTLLibrary> _GetLibrary() {
    PROFILE

    static id<MTLLibrary> const _library = [] {
        auto data = ::dispatch_data_create(Rocket_MetalLib, Rocket_MetalLib_len, nullptr, DISPATCH_DATA_DESTRUCTOR_DEFAULT);

        NSError* error = nil;
        id<MTLLibrary> library = [_GetDevice() newLibraryWithData: data error: &error];

        if (library == nil) {
            throw std::runtime_error([[error localizedDescription] UTF8String]);
        }

        return library;
    }();

    return _library;
}

static id<MTLCommandQueue> _GetQueue() {
    PROFILE

    return (__bridge id<MTLCommandQueue>)__GetDefaultGPUCommandQueue();
}

static id<MTLSamplerState> _CreateSampler(MTLSamplerMinMagFilter minFilter, MTLSamplerMinMagFilter magFilter) {
    PROFILE

    auto descriptor = [MTLSamplerDescriptor new];
    descriptor.minFilter = minFilter;
    descriptor.magFilter = magFilter;
    descriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
    descriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;

    return [_GetDevice() newSamplerStateWithDescriptor: descriptor];
}

static id<MTLRenderPipelineState> _CreatePipeline(NSString* fragmentName, MTLPixelFormat format, Blend blend) {
    PROFILE

    static id<MTLFunction> const _vertexFunction = [_GetLibrary() newFunctionWithName: @"quadVertex"];

    auto descriptor = [MTLRenderPipelineDescriptor new];
    descriptor.vertexFunction = _vertexFunction;
    descriptor.fragmentFunction = [_GetLibrary() newFunctionWithName: fragmentName];

    auto attachment = descriptor.colorAttachments[0];
    attachment.pixelFormat = format;
    _SetBlend(attachment, blend);

    NSError* error = nil;
    auto pipeline = [_GetDevice() newRenderPipelineStateWithDescriptor: descriptor error: &error];

    if (pipeline == nil) {
        throw std::runtime_error([[error localizedDescription] UTF8String]);
    }

    return pipeline;
}

static id<MTLRenderPipelineState> _GetPipeline(NSString* fragmentName, MTLPixelFormat format, Blend blend, std::map<std::uint64_t, id<MTLRenderPipelineState>>& cache) {
    PROFILE

    auto const key = (((std::uint64_t)format) << 8) | (std::uint64_t)blend;
    auto it = cache.find(key);
    if (it == cache.end()) {
        it = cache.emplace(key, _CreatePipeline(fragmentName, format, blend)).first;
    }

    return it->second;
}

static id<MTLRenderPipelineState> _GetColorBrushPipeline(MTLPixelFormat format, Blend blend) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"colorBrushFragment", format, blend, _pipelines);
}

static id<MTLRenderPipelineState> _GetImageBrushPipeline(MTLPixelFormat format, Blend blend) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"imageBrushFragment", format, blend, _pipelines);
}

static id<MTLRenderPipelineState> _GetLinearGradientBrushPipeline(MTLPixelFormat format, Blend blend) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"linearGradientBrushFragment", format, blend, _pipelines);
}

static id<MTLRenderPipelineState> _GetRadialGradientBrushPipeline(MTLPixelFormat format, Blend blend) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"radialGradientBrushFragment", format, blend, _pipelines);
}

static id<MTLRenderPipelineState> _GetBlurFilterPipeline(MTLPixelFormat format, Blend blend) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"blurFilterFragment", format, blend, _pipelines);
}

static id<MTLRenderPipelineState> _GetShadowFilterDilatePipeline(MTLPixelFormat format, Blend blend) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"shadowFilterDilateFragment", format, blend, _pipelines);
}

static id<MTLRenderPipelineState> _GetShadowFilterBlurPipeline(MTLPixelFormat format, Blend blend) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"shadowFilterBlurFragment", format, blend, _pipelines);
}

static id<MTLRenderPipelineState> _GetShadowFilterCompositePipeline(MTLPixelFormat format, Blend blend) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"shadowFilterCompositeFragment", format, blend, _pipelines);
}

static id<MTLSamplerState> _GetLinLinSampler() {
    PROFILE

    static id<MTLSamplerState> const _sampler = _CreateSampler(MTLSamplerMinMagFilterLinear, MTLSamplerMinMagFilterLinear);

    return _sampler;
}

static id<MTLSamplerState> _GetNerNerSampler() {
    PROFILE

    static id<MTLSamplerState> const _sampler = _CreateSampler(MTLSamplerMinMagFilterNearest, MTLSamplerMinMagFilterNearest);

    return _sampler;
}

static id<MTLSamplerState> _GetLinNerSampler() {
    PROFILE

    static id<MTLSamplerState> const _sampler = _CreateSampler(MTLSamplerMinMagFilterLinear, MTLSamplerMinMagFilterNearest);

    return _sampler;
}

static id<MTLSamplerState> _GetNerLinSampler() {
    PROFILE

    static id<MTLSamplerState> const _sampler = _CreateSampler(MTLSamplerMinMagFilterNearest, MTLSamplerMinMagFilterLinear);

    return _sampler;
}

static Vec2 _GetTexelSize(id<MTLTexture> texture) {
    return { (1.0f / static_cast<float>(texture.width)), (1.0f / static_cast<float>(texture.height)) };
}

static int _GetGaussianTaps(float extent) {
    return std::clamp(static_cast<int>(std::ceil(extent)), 0, _BLUR_TAP_MAX);
}

static void _BeginRenderPass(_RenderPass& renderPass, id<MTLCommandBuffer> command, id<MTLTexture> texture, id<CAMetalDrawable> drawable, std::uint32_t width, std::uint32_t height, MTLPixelFormat format, std::optional<Vec4> const& clear) {
    PROFILE

    assert(command != nil);
    assert(texture != nil);
    assert(width > 0);
    assert(height > 0);

    assert(renderPass.commandBuffer == nil);
    assert(renderPass.encoder == nil);
    assert(renderPass.texture == nil);

    auto descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    descriptor.colorAttachments[0].texture = texture;
    descriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;
    descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

    if (clear.has_value()) {
        descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        descriptor.colorAttachments[0].clearColor = ::MTLClearColorMake( /* the target is premultiplied; the clear color is straight RGBA */
            (clear->red * clear->alpha),
            (clear->green * clear->alpha),
            (clear->blue * clear->alpha),
            clear->alpha
        );
    }

    renderPass.width = width;
    renderPass.height = height;
    renderPass.format = format;
    renderPass.texture = texture;
    renderPass.drawable = drawable;
    renderPass.commandBuffer = command;
    renderPass.encoder = [command renderCommandEncoderWithDescriptor: descriptor];
}

static void _EndRenderPass(_RenderPass& renderPass) {
    PROFILE

    assert(renderPass.commandBuffer != nil);
    assert(renderPass.encoder != nil);
    assert(renderPass.texture != nil);

    [renderPass.encoder endEncoding];

    renderPass.width = 0;
    renderPass.height = 0;
    renderPass.texture = nil;
    renderPass.drawable = nil;
    renderPass.encoder = nil;
    renderPass.commandBuffer = nil;
    renderPass.format = MTLPixelFormatInvalid;
    renderPass.commit = false;
}

static void _SuspendRenderPass(_RenderPass& renderPass) {
    PROFILE

    assert(renderPass.commandBuffer != nil);
    assert(renderPass.encoder != nil);
    assert(renderPass.texture != nil);

    [renderPass.encoder endEncoding];

    renderPass.encoder = nil;
}

static void _ResumeRenderPass(_RenderPass& renderPass) {
    PROFILE

    assert(renderPass.commandBuffer != nil);
    assert(renderPass.texture != nil);

    auto descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    descriptor.colorAttachments[0].texture = renderPass.texture;
    descriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;
    descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

    renderPass.encoder = [renderPass.commandBuffer renderCommandEncoderWithDescriptor: descriptor];
}

static void _PushStateUniforms(_RenderPass const& renderPass, Vec2 const& resolution, Mat3 const& transform, float opacity) {
    PROFILE

    assert(renderPass.encoder != nil);

    auto const stateUniforms = StateUniforms{
        .resolution = simd_make_float2(resolution.x, resolution.y),
        .projection = simd_float3x3{
            simd_make_float3(2.0f / resolution.width, 0.0f, 0.0f),
            simd_make_float3(0.0f, -2.0f / resolution.height, 0.0f),
            simd_make_float3(-1.0f, 1.0f, 1.0f)
        },
        .transform = simd_matrix(
            simd_make_float3(transform.data[0], transform.data[1], transform.data[2]),
            simd_make_float3(transform.data[3], transform.data[4], transform.data[5]),
            simd_make_float3(transform.data[6], transform.data[7], transform.data[8])
        ),
        .opacity = opacity
    };

    [renderPass.encoder setVertexBytes: &stateUniforms length: sizeof(StateUniforms) atIndex: _STATE_VERTEX_UNIFORMS_INDEX];
    [renderPass.encoder setFragmentBytes: &stateUniforms length: sizeof(StateUniforms) atIndex: _STATE_FRAGMENT_UNIFORMS_INDEX];
}

static void _PushShapeUniforms(_RenderPass const& renderPass, Shape const& shape, Vec2 const& expand, bool fragment) {
    PROFILE

    assert(renderPass.encoder != nil);

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

    auto const shapeUniforms = ShapeUniforms{
        .vertices = {
            simd_make_float2((rect.x - expand.x),         (rect.y - expand.y)),
            simd_make_float2((rect.x - expand.x),         (rect.getMaxY() + expand.y)),
            simd_make_float2((rect.getMaxX() + expand.x), (rect.y - expand.y)),
            simd_make_float2((rect.getMaxX() + expand.x), (rect.getMaxY() + expand.y))
        },
        .bounds  = { rect.x, rect.y, rect.width, rect.height }, // unexpanded: localPos and localUV stay in shape space
        .borders = { borders.left, borders.top, borders.right, borders.bottom },
        .corners = { corners.data[0], corners.data[1], corners.data[2], corners.data[3] },
        .type    = type
    };

    [renderPass.encoder setVertexBytes: &shapeUniforms length: sizeof(ShapeUniforms) atIndex: _SHAPE_VERTEX_UNIFORMS_INDEX];

    if (fragment) {
        [renderPass.encoder setFragmentBytes: &shapeUniforms length: sizeof(ShapeUniforms) atIndex: _SHAPE_FRAGMENT_UNIFORMS_INDEX];
    }
}

static void _PushColorBrushUniforms(_RenderPass const& renderPass, ColorBrush const& colorBrush) {
    PROFILE

    assert(renderPass.encoder != nil);

    auto const colorUniforms = ColorBrushUniforms{
        .color = simd_make_float4(colorBrush.color.red, colorBrush.color.green, colorBrush.color.blue, colorBrush.color.alpha)
    };

    [renderPass.encoder setFragmentBytes: &colorUniforms length: sizeof(ColorBrushUniforms) atIndex: _BRUSH_FRAGMENT_UNIFORMS_INDEX];
}

static void _PushImageBrushUniforms(_RenderPass const& renderPass, ImageBrush const& imageBrush, Vec4 const& bounds) {
    PROFILE

    assert(renderPass.encoder != nil);
    assert(imageBrush.image != nullptr);

    auto const imageSize = imageBrush.image->getSize();
    auto const imageColor = imageBrush.color.value_or(Vec4{});
    auto const imageNPatch = imageBrush.nPatch.value_or(Vec4{});
    auto const filterMin = imageBrush.filterMin.value_or(ImageFilter::Nearest);
    auto const filterMag = imageBrush.filterMag.value_or(ImageFilter::Nearest);
    auto const positionX = imageBrush.positionX.value_or(ImagePosition::Stretch);
    auto const positionY = imageBrush.positionY.value_or(ImagePosition::Stretch);
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

    auto const imageUniforms = ImageBrushUniforms{
        .color  = { imageColor.red, imageColor.green, imageColor.blue, imageColor.alpha },
        .size   = { imageSize.width, imageSize.height },
        .flip   = { (flipX ? 1.0f : 0.0f), (flipY ? 1.0f : 0.0f) },
        .repeat = { (repeatX ? 1.0f : 0.0f), (repeatY ? 1.0f : 0.0f) },
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

    [renderPass.encoder setFragmentTexture: (__bridge id<MTLTexture>)imageBrush.image->getTexture() atIndex: 0];
    [renderPass.encoder setFragmentSamplerState: sampler atIndex: 0];
    [renderPass.encoder setFragmentBytes: &imageUniforms length: sizeof(ImageBrushUniforms) atIndex: _BRUSH_FRAGMENT_UNIFORMS_INDEX];
}

static void _PushGradientBrushUniforms(_RenderPass const& renderPass, GradientBrush const& gradientBrush) {
    PROFILE

    assert(renderPass.encoder != nil);

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
    auto const premultiply = [](Vec4 const& c) { return FLOAT4{ (c.red * c.alpha), (c.green * c.alpha), (c.blue * c.alpha), c.alpha }; };

    auto gradientUniforms = GradientBrushUniforms{
        .startPoint = { startPosition.x, startPosition.y },
        .startColor = premultiply(startColor),
        .stopPoint  = { stopPosition.x, stopPosition.y },
        .stopColor  = premultiply(stopColor),
        .stopCount  = stopCount
    };
    for (auto i = 0; i < stopCount; i++) {
        auto const& [ position, color ] = gradientBrush.stops[i];
        gradientUniforms.stopColors[i]   = premultiply(color);
        gradientUniforms.stopPosition[i] = position;
    }

    [renderPass.encoder setFragmentBytes: &gradientUniforms length: sizeof(GradientBrushUniforms) atIndex: _BRUSH_FRAGMENT_UNIFORMS_INDEX];
}

static void _PushBlurFilterUniforms(_RenderPass const& renderPass, id<MTLTexture> source, Vec2 const& direction, BlurFilter const& filter, Vec2 const& scale) {
    PROFILE

    auto const texelSize = _GetTexelSize(source);
    auto const axisScale = ((direction.x != 0.0f) ? scale.x : scale.y);

    auto const blurUniforms = BlurFilterUniforms{
        .texelSize = { texelSize.x, texelSize.y },
        .direction = { direction.x, direction.y },
        .sigma     = (filter.getSigma() * axisScale),
        .taps      = _GetGaussianTaps(filter.getExtent() * axisScale)
    };

    [renderPass.encoder setFragmentBytes: &blurUniforms length: sizeof(BlurFilterUniforms) atIndex: _BRUSH_FRAGMENT_UNIFORMS_INDEX];
}

static void _PushShadowFilterFieldUniforms(_RenderPass const& renderPass, id<MTLTexture> source, Vec2 const& direction, float spread, ShadowFilter const& filter, bool fromSilhouette, Vec2 const& scale) {
    PROFILE

    auto const texel = _GetTexelSize(source);
    auto const axisScale = ((direction.x != 0.0f) ? scale.x : scale.y);

    auto const fieldUniforms = ShadowFilterFieldUniforms{
        .texel          = { texel.x, texel.y },
        .direction      = { direction.x, direction.y },
        .spread         = std::clamp((spread * axisScale), -static_cast<float>(_BLUR_TAP_MAX), static_cast<float>(_BLUR_TAP_MAX)),
        .sigma          = (filter.getSigma() * axisScale),
        .taps           = _GetGaussianTaps(filter.getExtent() * axisScale),
        .fromSilhouette = (fromSilhouette ? 1 : 0),
        .invert         = ((fromSilhouette && filter.inset) ? 1 : 0)
    };

    [renderPass.encoder setFragmentBytes: &fieldUniforms length: sizeof(ShadowFilterFieldUniforms) atIndex: _BRUSH_FRAGMENT_UNIFORMS_INDEX];
}

static void _PushShadowFilterUniforms(_RenderPass const& renderPass, id<MTLTexture> source, ShadowFilter const& filter, bool fromSilhouette, Vec2 const& scale) {
    PROFILE

    auto const texel = _GetTexelSize(source);
    auto const offset = filter.offset.value_or(Vec2{});

    auto const shadowUniforms = ShadowFilterUniforms{
        .color          = { filter.color.red, filter.color.green, filter.color.blue, filter.color.alpha },
        .opacity        = 1.0f,
        .texel          = { texel.x, texel.y },
        .offset         = { (offset.x * scale.x), (offset.y * scale.y) },
        .sigma          = (filter.getSigma() * scale.y),
        .taps           = _GetGaussianTaps(filter.getExtent() * scale.y),
        .inset          = (filter.inset ? 1 : 0),
        .fromSilhouette = (fromSilhouette ? 1 : 0),
        .invert         = ((fromSilhouette && filter.inset) ? 1 : 0)
    };

    [renderPass.encoder setFragmentBytes: &shadowUniforms length: sizeof(ShadowFilterUniforms) atIndex: _BRUSH_FRAGMENT_UNIFORMS_INDEX];
}

void* __GetDefaultGPUDevice() {
    PROFILE

    static id<MTLDevice> const _device = ::MTLCreateSystemDefaultDevice();

    return (__bridge void*)_device;
}

void* __GetDefaultGPUCommandQueue() {
    PROFILE

    static id<MTLCommandQueue> const _queue = [(__bridge id<MTLDevice>)__GetDefaultGPUDevice() newCommandQueue];

    return (__bridge void*)_queue;
}

int __GetDefaultTextureFormat() {
    PROFILE

    return (int)MTLPixelFormatRGBA8Unorm;
}

_FilterTextures& Painter::_Painter::getFilterTextures(std::uint32_t width, std::uint32_t height) {
    PROFILE

    assert(width > 0);
    assert(height > 0);

    auto const key = ((static_cast<std::uint64_t>(width) << 32) | static_cast<std::uint64_t>(height));

    auto it = filterTextures.find(key);
    if (it == filterTextures.end()) {
        auto descriptor = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat: (MTLPixelFormat)__GetDefaultTextureFormat()
                                         width: width
                                        height: height
                                     mipmapped: NO];

        descriptor.usage = (MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget);
        descriptor.storageMode = MTLStorageModePrivate;

        it = filterTextures.emplace(key, _FilterTextures{
            0, { [_GetDevice() newTextureWithDescriptor: descriptor],
                 [_GetDevice() newTextureWithDescriptor: descriptor] },
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

        filterTextures.erase(evict); // ARC releases the textures
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
        if (renderPass.has_value() && (renderPass->encoder != nil)) {
            _EndRenderPass(*renderPass);
        }
        _impl->renderPassStack.pop();
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

    target.match(
        [&](ImagePaintTarget const& imageTarget) {
            assert(imageTarget.image.getTexture() != nullptr);

            _impl->renderPassStack.emplace(_RenderPass{});

            if (_impl->commandBuffer == nil) {
                _impl->commandBuffer = [_GetQueue() commandBuffer];
                _impl->renderPassStack.top()->commit = true;
            }

            _BeginRenderPass(
                *_impl->renderPassStack.top(),
                _impl->commandBuffer,
                (__bridge id<MTLTexture>)imageTarget.image.getTexture(),
                nil,
                (std::uint32_t)imageTarget.image.getSize().width,
                (std::uint32_t)imageTarget.image.getSize().height,
                (MTLPixelFormat)__GetDefaultTextureFormat(),
                imageTarget.clearColor
            );
        },
        [&](WindowPaintTarget const& windowTarget) {
            assert(windowTarget.window.getHandle() != nullptr);

            auto window = (__bridge NSWindow*)windowTarget.window.getHandle();
            auto layer = (CAMetalLayer*)window.contentView.layer;

            assert([layer isKindOfClass: [CAMetalLayer class]]);

            if (auto drawable = [layer nextDrawable]) {
                _impl->renderPassStack.emplace(_RenderPass{});

                if (_impl->commandBuffer == nil) {
                    _impl->commandBuffer = [_GetQueue() commandBuffer];
                    _impl->renderPassStack.top()->commit = true;
                }

                _BeginRenderPass(
                    *_impl->renderPassStack.top(),
                    _impl->commandBuffer,
                    drawable.texture,
                    drawable,
                    (std::uint32_t)layer.drawableSize.width,
                    (std::uint32_t)layer.drawableSize.height,
                    layer.pixelFormat,
                    windowTarget.clearColor
                );
            } else {
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

            if (renderPass.drawable != nil) {
                [_impl->commandBuffer presentDrawable: renderPass.drawable];
            }

            _EndRenderPass(renderPass);

            if (commit) {
                [_impl->commandBuffer commit];
                _impl->commandBuffer = nil;
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

    assert(_impl->renderPassStack.top()->encoder != nil);

    auto& renderPass = *_impl->renderPassStack.top();

    auto const colorBrush = brush.as<ColorBrush>();
    auto const imageBrush = brush.as<ImageBrush>();
    auto const gradientBrush = brush.as<GradientBrush>();
    auto const shadowFilter = options.filter.has_value() ? options.filter->as<ShadowFilter>() : nullptr;
    auto const blurFilter = options.filter.has_value() ? options.filter->as<BlurFilter>() : nullptr;
    auto const blend = options.blend.value_or(Blend::Over);

    auto const drawShape = [&](_RenderPass& pass, std::optional<Vec4> const& scissor, Blend mode, Vec2 const& scale) {
        assert(pass.encoder != nil);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        auto const transform = options.transform.value_or(Mat3{}).toScaled(scale);

        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, transform, options.opacity.value_or(1.0f));
        _PushShapeUniforms(pass, shape, _EdgeExpansion(transform), true);

        if (colorBrush != nullptr) {
            [pass.encoder setRenderPipelineState: _GetColorBrushPipeline(pass.format, mode)];
            _PushColorBrushUniforms(pass, *colorBrush);
        } else if (imageBrush != nullptr) {
            [pass.encoder setRenderPipelineState: _GetImageBrushPipeline(pass.format, mode)];
            _PushImageBrushUniforms(pass, *imageBrush, bounds);
        } else if (gradientBrush != nullptr) {
            if (gradientBrush->radial.value_or(false)) {
                [pass.encoder setRenderPipelineState: _GetRadialGradientBrushPipeline(pass.format, mode)];
            } else {
                [pass.encoder setRenderPipelineState: _GetLinearGradientBrushPipeline(pass.format, mode)];
            }
            _PushGradientBrushUniforms(pass, *gradientBrush);
        }

        [pass.encoder drawPrimitives: MTLPrimitiveTypeTriangleStrip vertexStart: 0 vertexCount: 4];
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

    auto const drawBlur = [&](_RenderPass& pass, id<MTLTexture> source, Vec2 const& direction, Vec4 const& rect, std::optional<Vec4> const& scissor, Blend mode) {
        assert(pass.encoder != nil);
        assert(source != nil);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, Mat3{}, 1.0f);
        _PushShapeUniforms(pass, Shape{ QuadShape{ rect } }, Vec2{}, false);
        _PushBlurFilterUniforms(pass, source, direction, *blurFilter, filterScale);

        [pass.encoder setRenderPipelineState: _GetBlurFilterPipeline(pass.format, mode)];
        [pass.encoder setFragmentTexture: source atIndex: 0];
        [pass.encoder setFragmentSamplerState: _GetLinLinSampler() atIndex: 0];
        [pass.encoder drawPrimitives: MTLPrimitiveTypeTriangleStrip vertexStart: 0 vertexCount: 4];
    };

    auto const drawShadowField = [&](_RenderPass& pass, id<MTLTexture> source, id<MTLRenderPipelineState> pipeline, Vec2 const& direction, float spread, bool fromSilhouette, Vec4 const& rect) {
        assert(pass.encoder != nil);
        assert(source != nil);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        _SetScissor(pass, rect);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, Mat3{}, 1.0f);
        _PushShapeUniforms(pass, Shape{ QuadShape{ rect } }, Vec2{}, false);
        _PushShadowFilterFieldUniforms(pass, source, direction, spread, *shadowFilter, fromSilhouette, filterScale);

        [pass.encoder setRenderPipelineState: pipeline];
        [pass.encoder setFragmentTexture: source atIndex: 0];
        [pass.encoder setFragmentSamplerState: _GetLinLinSampler() atIndex: 0];
        [pass.encoder drawPrimitives: MTLPrimitiveTypeTriangleStrip vertexStart: 0 vertexCount: 4];
    };

    auto const drawShadowComposite = [&](_RenderPass& pass, id<MTLTexture> source, bool fromSilhouette, Vec4 const& rect, std::optional<Vec4> const& scissor, Blend mode) {
        assert(pass.encoder != nil);
        assert(source != nil);

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

        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, transform, 1.0f);
        _PushShapeUniforms(pass, Shape{ QuadShape{ quad } }, Vec2{}, false);
        _PushShadowFilterUniforms(pass, source, *shadowFilter, fromSilhouette, filterScale);

        [pass.encoder setRenderPipelineState: _GetShadowFilterCompositePipeline(pass.format, mode)];
        [pass.encoder setFragmentTexture: source atIndex: 0];
        [pass.encoder setFragmentSamplerState: _GetLinLinSampler() atIndex: 0];
        [pass.encoder drawPrimitives: MTLPrimitiveTypeTriangleStrip vertexStart: 0 vertexCount: 4];
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
        if ((targetScissor.width <= 0.0f) || (targetScissor.height <= 0.0f)) {
            return;
        }

        auto const scratchRect = _ToFilterSpace(*filterBounds, filterScale);

        _SuspendRenderPass(renderPass);

        auto const& [ shapeTexture, blurTexture ] = _impl->getFilterTextures(filterWidth, filterHeight).textures;
        auto const filterFormat = (MTLPixelFormat)__GetDefaultTextureFormat();
        auto const clearColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

        auto shapePass = _RenderPass{};
        _BeginRenderPass(shapePass, renderPass.commandBuffer, shapeTexture, nil, filterWidth, filterHeight, filterFormat, clearColor);
        drawShape(shapePass, std::nullopt, Blend::Over, filterScale);
        _EndRenderPass(shapePass);

        auto blurPass = _RenderPass{};
        _BeginRenderPass(blurPass, renderPass.commandBuffer, blurTexture, nil, filterWidth, filterHeight, filterFormat, clearColor);
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
        if ((targetScissor.width <= 0.0f) || (targetScissor.height <= 0.0f)) {
            return;
        }

        auto const scratchRect = _ToFilterSpace(*filterBounds, filterScale);

        _SuspendRenderPass(renderPass);

        auto [ source, target ] = _impl->getFilterTextures(filterWidth, filterHeight).textures;
        auto const filterFormat = (MTLPixelFormat)__GetDefaultTextureFormat();
        auto const clearColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
        auto const spread = shadowFilter->spread.value_or(Vec2{});
        auto fromSilhouette = true;

        auto shapePass = _RenderPass{};
        _BeginRenderPass(shapePass, renderPass.commandBuffer, source, nil, filterWidth, filterHeight, filterFormat, clearColor);
        drawShape(shapePass, std::nullopt, Blend::Over, filterScale);
        _EndRenderPass(shapePass);

        auto const fieldPass = [&](id<MTLRenderPipelineState> pipeline, Vec2 const& direction, float extent) {
            auto pass = _RenderPass{};
            _BeginRenderPass(pass, renderPass.commandBuffer, target, nil, filterWidth, filterHeight, filterFormat, clearColor);
            drawShadowField(pass, source, pipeline, direction, extent, fromSilhouette, scratchRect);
            _EndRenderPass(pass);
            std::swap(source, target);
            fromSilhouette = false;
        };

        if (spread.x != 0.0f) {
            fieldPass(_GetShadowFilterDilatePipeline(filterFormat, Blend::Over), Vec2{ 1.0f, 0.0f }, spread.x);
        }
        if (spread.y != 0.0f) {
            fieldPass(_GetShadowFilterDilatePipeline(filterFormat, Blend::Over), Vec2{ 0.0f, 1.0f }, spread.y);
        }
        if (shadowFilter->getExtent() > 0.0f) {
            fieldPass(_GetShadowFilterBlurPipeline(filterFormat, Blend::Over), Vec2{ 1.0f, 0.0f }, 0.0f);
        }

        _ResumeRenderPass(renderPass);
        drawShadowComposite(renderPass, source, fromSilhouette, *filterBounds, targetScissor, blend);
    }
}

} /* namespace Rocket */
