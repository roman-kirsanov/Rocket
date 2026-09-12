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
#include "Metal/Painter.metal-lib.hpp"

namespace Rocket {

struct _RenderPass {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    id<MTLTexture> texture = nil;
    id<CAMetalDrawable> drawable = nil; // window targets only; presented on submit
    id<MTLCommandBuffer> commandBuffer = nil;
    id<MTLRenderCommandEncoder> encoder = nil;
    id<MTLRenderPipelineState> quadColorPipeline = nil;
    id<MTLRenderPipelineState> quadImagePipeline = nil;
    id<MTLRenderPipelineState> quadLinearGradientPipeline = nil;
    id<MTLRenderPipelineState> quadRadialGradientPipeline = nil;
    id<MTLRenderPipelineState> shadowPass1Pipeline = nil;
    id<MTLRenderPipelineState> shadowPass2Pipeline = nil;
    id<MTLRenderPipelineState> blurPipeline = nil;
};

struct _FilterTextures {
    std::uint64_t lastUse = 0;
    std::tuple<
        id<MTLTexture>,
        id<MTLTexture>
    > textures;
};

struct Painter::_Painter {
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

static id<MTLRenderPipelineState> _CreatePipeline(NSString* fragmentName, MTLPixelFormat format) {
    PROFILE

    static id<MTLFunction> const _vertexFunction = [_GetLibrary() newFunctionWithName: @"quadVertex"];

    auto descriptor = [MTLRenderPipelineDescriptor new];
    descriptor.vertexFunction = _vertexFunction;
    descriptor.fragmentFunction = [_GetLibrary() newFunctionWithName: fragmentName];

    auto attachment = descriptor.colorAttachments[0];
    attachment.pixelFormat = format;
    // Premultiplied-alpha "over": fragments output premultiplied color.
    attachment.blendingEnabled = YES;
    attachment.sourceRGBBlendFactor = MTLBlendFactorOne;
    attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    attachment.rgbBlendOperation = MTLBlendOperationAdd;
    attachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
    attachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    attachment.alphaBlendOperation = MTLBlendOperationAdd;

    NSError* error = nil;
    auto pipeline = [_GetDevice() newRenderPipelineStateWithDescriptor: descriptor error: &error];

    if (pipeline == nil) {
        throw std::runtime_error([[error localizedDescription] UTF8String]);
    }

    return pipeline;
}

static id<MTLRenderPipelineState> _GetPipeline(NSString* fragmentName, MTLPixelFormat format, std::map<std::uint64_t, id<MTLRenderPipelineState>>& cache) {
    PROFILE

    auto it = cache.find((std::uint64_t)format);
    if (it == cache.end()) {
        it = cache.emplace((std::uint64_t)format, _CreatePipeline(fragmentName, format)).first;
    }

    return it->second;
}

static id<MTLRenderPipelineState> _GetQuadColorPipeline(MTLPixelFormat format) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"colorFragment", format, _pipelines);
}

static id<MTLRenderPipelineState> _GetQuadImagePipeline(MTLPixelFormat format) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"imageFragment", format, _pipelines);
}

static id<MTLRenderPipelineState> _GetQuadLinearGradientPipeline(MTLPixelFormat format) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"linearGradientFragment", format, _pipelines);
}

static id<MTLRenderPipelineState> _GetQuadRadialGradientPipeline(MTLPixelFormat format) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"radialGradientFragment", format, _pipelines);
}

static id<MTLRenderPipelineState> _GetBlurPipeline(MTLPixelFormat format) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"blurFragment", format, _pipelines);
}

static id<MTLRenderPipelineState> _GetShadowPass1Pipeline(MTLPixelFormat format) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"shadowPass1Fragment", format, _pipelines);
}

static id<MTLRenderPipelineState> _GetShadowPass2Pipeline(MTLPixelFormat format) {
    PROFILE

    static auto _pipelines = std::map<std::uint64_t, id<MTLRenderPipelineState>>();

    return _GetPipeline(@"shadowPass2Fragment", format, _pipelines);
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
        descriptor.colorAttachments[0].clearColor = ::MTLClearColorMake(clear->red, clear->green, clear->blue, clear->alpha);
    }

    renderPass.width = width;
    renderPass.height = height;
    renderPass.texture = texture;
    renderPass.drawable = drawable;
    renderPass.commandBuffer = command;
    renderPass.encoder = [command renderCommandEncoderWithDescriptor: descriptor];
    renderPass.quadColorPipeline = _GetQuadColorPipeline(format);
    renderPass.quadImagePipeline = _GetQuadImagePipeline(format);
    renderPass.quadLinearGradientPipeline = _GetQuadLinearGradientPipeline(format);
    renderPass.quadRadialGradientPipeline = _GetQuadRadialGradientPipeline(format);
    renderPass.shadowPass1Pipeline = _GetShadowPass1Pipeline(format);
    renderPass.shadowPass2Pipeline = _GetShadowPass2Pipeline(format);
    renderPass.blurPipeline = _GetBlurPipeline(format);
}

static void _EndRenderPass(_RenderPass& renderPass, bool submit = false) {
    PROFILE

    assert(renderPass.commandBuffer != nil);
    assert(renderPass.encoder != nil);
    assert(renderPass.texture != nil);

    [renderPass.encoder endEncoding];

    if (submit) {
        if (renderPass.drawable != nil) {
            [renderPass.commandBuffer presentDrawable: renderPass.drawable];
        }

        [renderPass.commandBuffer commit];
    }

    renderPass.width = 0;
    renderPass.height = 0;
    renderPass.texture = nil;
    renderPass.drawable = nil;
    renderPass.encoder = nil;
    renderPass.commandBuffer = nil;
    renderPass.quadColorPipeline = nil;
    renderPass.quadImagePipeline = nil;
    renderPass.quadLinearGradientPipeline = nil;
    renderPass.quadRadialGradientPipeline = nil;
    renderPass.shadowPass1Pipeline = nil;
    renderPass.shadowPass2Pipeline = nil;
    renderPass.blurPipeline = nil;
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

static void _SetScissor(_RenderPass const& renderPass, std::optional<Vec4> const& scissor) {
    PROFILE

    assert(renderPass.encoder != nil);

    auto const passWidth = static_cast<int>(renderPass.width);
    auto const passHeight = static_cast<int>(renderPass.height);

    auto scissorInfo = MTLScissorRect{};

    if (scissor.has_value()) {
        auto const left = std::max(0, static_cast<int>(scissor->x));
        auto const top = std::max(0, static_cast<int>(scissor->y));
        auto const right = std::min(passWidth, static_cast<int>(scissor->x + scissor->width));
        auto const bottom = std::min(passHeight, static_cast<int>(scissor->y + scissor->height));

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

// Resolves a quad's per-corner radii (top-left, top-right, bottom-right,
// bottom-left), applying the uniform borderRadius as the fallback for any
// corner without an explicit override. Negative radii floor to 0, then the
// CSS corner-overlap rule (css-backgrounds-3) applies: all four radii are
// scaled by f = min(1, w/(TL+TR), w/(BL+BR), h/(TL+BL), h/(TR+BR)), skipping
// any ratio with a zero denominator. A lone corner may thus grow up to the
// full shorter side, while competing adjacent radii shrink proportionally
// instead of truncating per corner.
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

static void _PushShapeUniforms(_RenderPass const& renderPass, Shape const& shape, bool fragment) {
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
            simd_make_float2(rect.x, rect.y),
            simd_make_float2(rect.x, rect.getMaxY()),
            simd_make_float2(rect.getMaxX(), rect.y),
            simd_make_float2(rect.getMaxX(), rect.getMaxY())
        },
        .bounds  = { rect.x, rect.y, rect.width, rect.height },
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

    auto const colorUniforms = ColorUniforms{
        .color = simd_make_float4(colorBrush.color.red, colorBrush.color.green, colorBrush.color.blue, colorBrush.color.alpha)
    };

    [renderPass.encoder setFragmentBytes: &colorUniforms length: sizeof(ColorUniforms) atIndex: _BRUSH_FRAGMENT_UNIFORMS_INDEX];
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

    auto const imageUniforms = ImageUniforms{
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
    [renderPass.encoder setFragmentBytes: &imageUniforms length: sizeof(ImageUniforms) atIndex: _BRUSH_FRAGMENT_UNIFORMS_INDEX];
}

static void _PushGradiantBrushUniforms(_RenderPass const& renderPass, GradientBrush const& gradientBrush) {
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

    auto gradientUniforms = GradientUniforms{
        .startPoint = { startPosition.x, startPosition.y },
        .startColor = { startColor.red, startColor.green, startColor.blue, startColor.alpha },
        .stopPoint  = { stopPosition.x, stopPosition.y },
        .stopColor  = { stopColor.red, stopColor.green, stopColor.blue, stopColor.alpha },
        .stopCount  = stopCount
    };
    for (auto i = 0; i < stopCount; i++) {
        auto const& [ position, color ] = gradientBrush.stops[i];
        gradientUniforms.stopColors[i]   = { color.red, color.green, color.blue, color.alpha };
        gradientUniforms.stopPosition[i] = position;
    }

    [renderPass.encoder setFragmentBytes: &gradientUniforms length: sizeof(GradientUniforms) atIndex: _BRUSH_FRAGMENT_UNIFORMS_INDEX];
}

static void _PushBlurUniforms(_RenderPass const& renderPass, Vec2 const& direction, float radius) {
    PROFILE

    auto const blurUniforms = FilterBlurUniforms{
        .texelSize = { (1.0f / static_cast<float>(renderPass.width)), (1.0f / static_cast<float>(renderPass.height)) },
        .direction = { direction.x, direction.y },
        .sigma     = (radius / 2.0f),
        .taps      = std::min(static_cast<int>(std::ceil(radius)), _BLUR_TAP_MAX)
    };

    [renderPass.encoder setFragmentBytes: &blurUniforms length: sizeof(FilterBlurUniforms) atIndex: _BRUSH_FRAGMENT_UNIFORMS_INDEX];
}

static void _PushShadowPass1Uniforms(_RenderPass const& renderPass, float radius, bool inset) {
    PROFILE

    auto const shadowUniforms = BlurUniforms{
        .texel  = { (1.0f / static_cast<float>(renderPass.width)), (1.0f / static_cast<float>(renderPass.height)) },
        .radius = std::clamp(static_cast<int>(std::ceil(radius)), 0, _BLUR_TAP_MAX),
        .inset  = (inset ? 1 : 0)
    };

    [renderPass.encoder setFragmentBytes: &shadowUniforms length: sizeof(BlurUniforms) atIndex: _BRUSH_FRAGMENT_UNIFORMS_INDEX];
}

static void _PushShadowPass2Uniforms(_RenderPass const& renderPass, ShadowFilter const& filter) {
    PROFILE

    auto const spread = filter.spread.value_or(Vec2{});
    auto const offset = filter.offset.value_or(Vec2{});

    auto const shadowUniforms = ShadowUniforms{
        .color   = { filter.color.red, filter.color.green, filter.color.blue, filter.color.alpha },
        .opacity = 1.0f,
        .spread  = { spread.x, spread.y },
        .texel   = { (1.0f / static_cast<float>(renderPass.width)), (1.0f / static_cast<float>(renderPass.height)) },
        .offset  = { offset.x, offset.y },
        .radius  = std::clamp(static_cast<int>(std::ceil(filter.radius)), 0, _BLUR_TAP_MAX),
        .inset   = (filter.inset ? 1 : 0)
    };

    [renderPass.encoder setFragmentBytes: &shadowUniforms length: sizeof(ShadowUniforms) atIndex: _BRUSH_FRAGMENT_UNIFORMS_INDEX];
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

    if (_impl != nullptr) {
        delete _impl;
        _impl = nullptr;
    }
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

            _impl->renderPassStack.emplace();
            _impl->renderPassStack.top().emplace();

            _BeginRenderPass(
                *_impl->renderPassStack.top(),
                [_GetQueue() commandBuffer],
                (__bridge id<MTLTexture>)imageTarget.image.getTexture(),
                nil,
                (std::uint32_t)imageTarget.image.getSize().width,
                (std::uint32_t)imageTarget.image.getSize().height,
                (MTLPixelFormat)__GetDefaultTextureFormat(),
                imageTarget.clearColor
            );
        },
        [&](WindowPaintTarget const& windowTarget) {
            auto window = (__bridge NSWindow*)windowTarget.window.getHandle();
            auto layer = (CAMetalLayer*)window.contentView.layer;

            assert([layer isKindOfClass: [CAMetalLayer class]]);

            auto drawable = [layer nextDrawable];
            if (drawable != nil) {
                _impl->renderPassStack.emplace();
                _impl->renderPassStack.top().emplace();

                _BeginRenderPass(
                    *_impl->renderPassStack.top(),
                    [_GetQueue() commandBuffer],
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
            _EndRenderPass(*_impl->renderPassStack.top(), true);
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

void Painter::__paint(Shape const& shape, Brush const& brush, PaintOptions const& options, Vec4 const& bounds) {
    PROFILE

    if (_impl->renderPassStack.empty()) {
        return;
    }

    if (_impl->renderPassStack.top().has_value() == false) {
        return;
    }

    assert(_impl->renderPassStack.top()->encoder != nil);

    auto const colorBrush = brush.as<ColorBrush>();
    auto const imageBrush = brush.as<ImageBrush>();
    auto const gradientBrush = brush.as<GradientBrush>();
    auto const blurFilter = options.filter.has_value() ? options.filter->as<BlurFilter>() : nullptr;
    auto const shadowFilter = options.filter.has_value() ? options.filter->as<ShadowFilter>() : nullptr;
    auto& renderPass = *_impl->renderPassStack.top();

    auto const drawShape = [&](_RenderPass& pass, std::optional<Vec4> const& scissor) {
        assert(pass.encoder != nil);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, options.transform.value_or(Mat3{}), options.opacity.value_or(1.0f));
        _PushShapeUniforms(pass, shape, true);

        if (colorBrush != nullptr) {
            [pass.encoder setRenderPipelineState: pass.quadColorPipeline];
            _PushColorBrushUniforms(pass, *colorBrush);
        } else if (imageBrush != nullptr) {
            [pass.encoder setRenderPipelineState: pass.quadImagePipeline];
            _PushImageBrushUniforms(pass, *imageBrush, bounds);
        } else if (gradientBrush != nullptr) {
            if (gradientBrush->radial.value_or(false)) {
                [pass.encoder setRenderPipelineState: pass.quadRadialGradientPipeline];
            } else {
                [pass.encoder setRenderPipelineState: pass.quadLinearGradientPipeline];
            }
            _PushGradiantBrushUniforms(pass, *gradientBrush);
        }

        [pass.encoder drawPrimitives: MTLPrimitiveTypeTriangleStrip vertexStart: 0 vertexCount: 4];
    };

    auto const drawBlur = [&](_RenderPass& pass, id<MTLTexture> source, Vec2 const& direction, std::optional<Vec4> const& scissor) {
        assert(pass.encoder != nil);
        assert(source != nil);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, Mat3{}, 1.0f);
        _PushShapeUniforms(pass, Shape{ QuadShape{ Vec4{ {}, resolution } } }, false);
        _PushBlurUniforms(pass, direction, blurFilter->radius);

        [pass.encoder setRenderPipelineState: pass.blurPipeline];
        [pass.encoder setFragmentTexture: source atIndex: 0];
        [pass.encoder setFragmentSamplerState: _GetLinLinSampler() atIndex: 0];
        [pass.encoder drawPrimitives: MTLPrimitiveTypeTriangleStrip vertexStart: 0 vertexCount: 4];
    };

    auto const drawShadowPass1 = [&](_RenderPass& pass, id<MTLTexture> source, std::optional<Vec4> const& scissor) {
        assert(pass.encoder != nil);
        assert(source != nil);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, Mat3{}, 1.0f);
        _PushShapeUniforms(pass, Shape{ QuadShape{ Vec4{ {}, resolution } } }, false);
        _PushShadowPass1Uniforms(pass, shadowFilter->radius, shadowFilter->inset);

        [pass.encoder setRenderPipelineState: pass.shadowPass1Pipeline];
        [pass.encoder setFragmentTexture: source atIndex: 0];
        [pass.encoder setFragmentSamplerState: _GetLinLinSampler() atIndex: 0];
        [pass.encoder drawPrimitives: MTLPrimitiveTypeTriangleStrip vertexStart: 0 vertexCount: 4];
    };

    auto const drawShadowPass2 = [&](_RenderPass& pass, id<MTLTexture> source, id<MTLTexture> mask, std::optional<Vec4> const& scissor) {
        assert(pass.encoder != nil);
        assert(source != nil);
        assert(mask != nil);

        auto const resolution = Vec2{
            static_cast<float>(pass.width),
            static_cast<float>(pass.height)
        };

        // A drop shadow is displaced by translating the composite quad; an inset shadow
        // keeps the quad in place and shifts the sampled field in the shader instead, so
        // the silhouette mask stays aligned with the shape.
        auto const transform = shadowFilter->inset
            ? Mat3{}
            : Mat3{}.toTranslated(shadowFilter->offset.value_or(Vec2{}));

        _SetScissor(pass, scissor);
        _SetViewport(pass, { {}, resolution });
        _PushStateUniforms(pass, resolution, transform, 1.0f);
        _PushShapeUniforms(pass, Shape{ QuadShape{ Vec4{ {}, resolution } } }, false);
        _PushShadowPass2Uniforms(pass, *shadowFilter);

        [pass.encoder setRenderPipelineState: pass.shadowPass2Pipeline];
        [pass.encoder setFragmentTexture: source atIndex: 0];
        [pass.encoder setFragmentTexture: mask atIndex: 1];
        [pass.encoder setFragmentSamplerState: _GetLinLinSampler() atIndex: 0];
        [pass.encoder drawPrimitives: MTLPrimitiveTypeTriangleStrip vertexStart: 0 vertexCount: 4];
    };

    if (
        (blurFilter != nullptr) &&
        (blurFilter->radius > 0.0f)
    ) {
        _SuspendRenderPass(renderPass);

        auto const& [ shapeTexture, blurTexture ] = _impl->getFilterTextures(renderPass.width, renderPass.height).textures;
        auto const filterFormat = (MTLPixelFormat)__GetDefaultTextureFormat();
        auto const clearColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

        auto shapePass = _RenderPass{};
        _BeginRenderPass(shapePass, renderPass.commandBuffer, shapeTexture, nil, renderPass.width, renderPass.height, filterFormat, clearColor);
        drawShape(shapePass, std::nullopt);
        _EndRenderPass(shapePass);

        auto blurPass = _RenderPass{};
        _BeginRenderPass(blurPass, renderPass.commandBuffer, blurTexture, nil, renderPass.width, renderPass.height, filterFormat, clearColor);
        drawBlur(blurPass, shapeTexture, Vec2{ 1.0f, 0.0f }, std::nullopt);
        _EndRenderPass(blurPass);

        _ResumeRenderPass(renderPass);
        drawBlur(renderPass, blurTexture, Vec2{ 0.0f, 1.0f }, options.scissor);
    } else if (shadowFilter != nullptr) {
        _SuspendRenderPass(renderPass);

        auto const& [ shapeTexture, shadowTexture ] = _impl->getFilterTextures(renderPass.width, renderPass.height).textures;
        auto const filterFormat = (MTLPixelFormat)__GetDefaultTextureFormat();
        auto const clearColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

        auto shapePass = _RenderPass{};
        _BeginRenderPass(shapePass, renderPass.commandBuffer, shapeTexture, nil, renderPass.width, renderPass.height, filterFormat, clearColor);
        drawShape(shapePass, std::nullopt);
        _EndRenderPass(shapePass);

        auto shadowPass = _RenderPass{};
        _BeginRenderPass(shadowPass, renderPass.commandBuffer, shadowTexture, nil, renderPass.width, renderPass.height, filterFormat, clearColor);
        drawShadowPass1(shadowPass, shapeTexture, std::nullopt);
        _EndRenderPass(shadowPass);

        _ResumeRenderPass(renderPass);
        drawShadowPass2(renderPass, shadowTexture, shapeTexture, options.scissor);
    } else {
        drawShape(renderPass, options.scissor);
    }
}

} /* namespace Rocket */
