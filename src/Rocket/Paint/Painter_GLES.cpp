/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <map>
#include <array>
#include <cmath>
#include <stack>
#include <tuple>
#include <string>
#include <vector>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <variant>
#include <optional>
#include <algorithm>
#include <stdexcept>
#include <SDL3/SDL.h>
#define GL_GLES_PROTOTYPES 0
#include <GLES3/gl3.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Painter.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Painter_GLES.hpp>
#include <Rocket/Paint/Painter_Private.hpp>
#include <Rocket/Paint/Painter_SDL3.hpp>
#include <Rocket/Window/App_SDL3.hpp>
#include <Rocket/Window/Window.hpp>
#include "Painter.glsl.hpp"
#include "QuadVertex.glsl.hpp"
#include "ColorBrush.glsl.hpp"
#include "ImageBrush.glsl.hpp"
#include "LinearGradientBrush.glsl.hpp"
#include "RadialGradientBrush.glsl.hpp"
#include "BlurFilter.glsl.hpp"
#include "ShadowFilterDilate.glsl.hpp"
#include "ShadowFilterBlur.glsl.hpp"
#include "ShadowFilterComposite.glsl.hpp"

namespace Rocket {

/* The uniform blocks of GLES/Painter.glsl in std140 layout: vec2 aligns to 8
   bytes, vec4, mat3 columns and array elements to 16, so the padding is spelled
   out and the offsets are checked below. */

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

/** Colors are premultiplied. */
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

/** The OpenGL ES 3.0 entry points the painter uses, loaded through SDL once the context exists. */
struct _GL {
    PFNGLACTIVETEXTUREPROC ActiveTexture = nullptr;
    PFNGLATTACHSHADERPROC AttachShader = nullptr;
    PFNGLBINDBUFFERPROC BindBuffer = nullptr;
    PFNGLBINDBUFFERRANGEPROC BindBufferRange = nullptr;
    PFNGLBINDFRAMEBUFFERPROC BindFramebuffer = nullptr;
    PFNGLBINDSAMPLERPROC BindSampler = nullptr;
    PFNGLBINDTEXTUREPROC BindTexture = nullptr;
    PFNGLBINDVERTEXARRAYPROC BindVertexArray = nullptr;
    PFNGLBLENDFUNCSEPARATEPROC BlendFuncSeparate = nullptr;
    PFNGLBLITFRAMEBUFFERPROC BlitFramebuffer = nullptr;
    PFNGLBUFFERDATAPROC BufferData = nullptr;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus = nullptr;
    PFNGLCLEARPROC Clear = nullptr;
    PFNGLCLEARCOLORPROC ClearColor = nullptr;
    PFNGLCOMPILESHADERPROC CompileShader = nullptr;
    PFNGLCREATEPROGRAMPROC CreateProgram = nullptr;
    PFNGLCREATESHADERPROC CreateShader = nullptr;
    PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers = nullptr;
    PFNGLDELETESHADERPROC DeleteShader = nullptr;
    PFNGLDELETETEXTURESPROC DeleteTextures = nullptr;
    PFNGLDISABLEPROC Disable = nullptr;
    PFNGLDRAWARRAYSPROC DrawArrays = nullptr;
    PFNGLENABLEPROC Enable = nullptr;
    PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D = nullptr;
    PFNGLGENBUFFERSPROC GenBuffers = nullptr;
    PFNGLGENFRAMEBUFFERSPROC GenFramebuffers = nullptr;
    PFNGLGENSAMPLERSPROC GenSamplers = nullptr;
    PFNGLGENTEXTURESPROC GenTextures = nullptr;
    PFNGLGENVERTEXARRAYSPROC GenVertexArrays = nullptr;
    PFNGLGETINTEGERVPROC GetIntegerv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog = nullptr;
    PFNGLGETPROGRAMIVPROC GetProgramiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog = nullptr;
    PFNGLGETSHADERIVPROC GetShaderiv = nullptr;
    PFNGLGETUNIFORMBLOCKINDEXPROC GetUniformBlockIndex = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation = nullptr;
    PFNGLLINKPROGRAMPROC LinkProgram = nullptr;
    PFNGLPIXELSTOREIPROC PixelStorei = nullptr;
    PFNGLREADPIXELSPROC ReadPixels = nullptr;
    PFNGLSAMPLERPARAMETERIPROC SamplerParameteri = nullptr;
    PFNGLSCISSORPROC Scissor = nullptr;
    PFNGLSHADERSOURCEPROC ShaderSource = nullptr;
    PFNGLTEXPARAMETERIPROC TexParameteri = nullptr;
    PFNGLTEXSTORAGE2DPROC TexStorage2D = nullptr;
    PFNGLTEXSUBIMAGE2DPROC TexSubImage2D = nullptr;
    PFNGLUNIFORM1IPROC Uniform1i = nullptr;
    PFNGLUNIFORMBLOCKBINDINGPROC UniformBlockBinding = nullptr;
    PFNGLUSEPROGRAMPROC UseProgram = nullptr;
    PFNGLVIEWPORTPROC Viewport = nullptr;
};

/** The one GL context every painter, image and window shares, owned by a hidden window of its own. */
struct _Context {
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;
    GLuint vertexArray = 0;
    GLuint uniformBuffer = 0;
    std::size_t uniformAlignment = 16;
};

struct _Texture {
    GLuint texture = 0;
    GLuint framebuffer = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

/** What a recorded command draws into: an image's texture, or a window's own framebuffer. */
struct _Target {
    _Texture* texture = nullptr;
    SDL_WindowID window = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    bool operator==(_Target const&) const = default;
};

/** A run of the uniform data recorded for a pass, bound to a block when its draw is replayed. */
struct _UniformRange {
    std::size_t offset = 0;
    std::size_t size = 0;
};

struct _Draw {
    _Target target;
    _Fragment fragment = _Fragment::ColorBrush;
    Blend blend = Blend::Over;
    std::array<GLint, 4> scissor = {};
    _Texture* texture = nullptr;
    GLuint sampler = 0;
    _UniformRange state;
    _UniformRange shape;
    _UniformRange brush;
};

struct _Clear {
    _Target target;
    std::array<float, 4> color = {};
};

using _Command = std::variant<_Draw, _Clear>;

struct _FilterTextures {
    std::uint64_t lastUse = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::tuple<
        _Texture*,
        _Texture*
    > textures;
};

/* GL runs a draw as soon as it is issued, but a pass must leave its target
   untouched until the outermost endPaint(), and not at all when the painter is
   destroyed first. So a painter records its passes as commands, with their
   uniforms, and replays them at the outermost endPaint(); dropping them
   abandons the passes. */
struct Painter::_Painter {
    std::stack<std::optional<_Target>> passStack;
    std::vector<_Command> commands;
    std::vector<std::uint8_t> uniforms;
    std::vector<SDL_WindowID> windows; // the windows painted in the recorded passes, presented after the replay
    std::map<std::uint64_t, _FilterTextures> filterTextures; // keyed by (width << 32) | height
    std::uint64_t filterTexturesUse = 0;
    _FilterTextures& getFilterTextures(std::uint32_t width, std::uint32_t height);
    void evictFilterTextures();
    template <typename Uniforms>
    _UniformRange pushUniforms(Uniforms const& uniforms);
    void replay();
    void drop();
};

static auto constexpr _BLUR_TAP_MAX = 64;
static auto constexpr _FILTER_TEXTURE_CACHE_MAX = 8;
static auto constexpr _STATE_UNIFORMS_BINDING = 0;
static auto constexpr _SHAPE_UNIFORMS_BINDING = 1;
static auto constexpr _BRUSH_UNIFORMS_BINDING = 2;

static auto _gl = _GL{};

/* how many painters hold recorded passes, and the textures released while any
   does: those are deleted once the last recording has run or been dropped */
static auto _recordingPainters = 0;
static auto _releasedTextures = std::vector<_Texture*>();

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

static void _SetBlend(Blend blend) {
    PROFILE

    switch (blend) {
        case Blend::Over:
            _gl.BlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case Blend::Add:
            // dst = src + dst; the target's alpha is kept
            _gl.BlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
            break;
        case Blend::Multiply:
            // dst = src * dst + dst * (1 - srcA): where the source is transparent the target stays
            _gl.BlendFuncSeparate(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
            break;
        case Blend::Screen:
            _gl.BlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_COLOR, GL_ZERO, GL_ONE);
            break;
    }
}

/* The scissor in GL's window coordinates: an image target's rows run top down,
   as its pixels are uploaded, but a window's framebuffer has its first row at
   the bottom. */
static std::array<GLint, 4> _GetScissor(_Target const& target, std::optional<Vec4> const& scissor) {
    PROFILE

    auto const passWidth = static_cast<int>(target.width);
    auto const passHeight = static_cast<int>(target.height);

    auto rect = std::array<GLint, 4>{ 0, 0, passWidth, passHeight };

    if (scissor.has_value()) {
        auto const left = std::max(0, static_cast<int>(std::roundf(scissor->x)));
        auto const top = std::max(0, static_cast<int>(std::roundf(scissor->y)));
        auto const right = std::min(passWidth, static_cast<int>(std::roundf(scissor->x + scissor->width)));
        auto const bottom = std::min(passHeight, static_cast<int>(std::roundf(scissor->y + scissor->height)));

        rect = {
            std::min(left, passWidth),
            std::min(top, passHeight),
            std::max(0, right - left),
            std::max(0, bottom - top)
        };
    }

    if (target.texture == nullptr) {
        rect[1] = (passHeight - (rect[1] + rect[3]));
    }

    return rect;
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

template <typename Function>
static void _LoadFunction(Function& function, char const* name) {
    PROFILE

    function = reinterpret_cast<Function>(::SDL_GL_GetProcAddress(name));

    if (function == nullptr) {
        throw std::runtime_error(std::string("Failed to load ") + name);
    }
}

static void _LoadFunctions() {
    PROFILE

    _LoadFunction(_gl.ActiveTexture, "glActiveTexture");
    _LoadFunction(_gl.AttachShader, "glAttachShader");
    _LoadFunction(_gl.BindBuffer, "glBindBuffer");
    _LoadFunction(_gl.BindBufferRange, "glBindBufferRange");
    _LoadFunction(_gl.BindFramebuffer, "glBindFramebuffer");
    _LoadFunction(_gl.BindSampler, "glBindSampler");
    _LoadFunction(_gl.BindTexture, "glBindTexture");
    _LoadFunction(_gl.BindVertexArray, "glBindVertexArray");
    _LoadFunction(_gl.BlendFuncSeparate, "glBlendFuncSeparate");
    _LoadFunction(_gl.BlitFramebuffer, "glBlitFramebuffer");
    _LoadFunction(_gl.BufferData, "glBufferData");
    _LoadFunction(_gl.CheckFramebufferStatus, "glCheckFramebufferStatus");
    _LoadFunction(_gl.Clear, "glClear");
    _LoadFunction(_gl.ClearColor, "glClearColor");
    _LoadFunction(_gl.CompileShader, "glCompileShader");
    _LoadFunction(_gl.CreateProgram, "glCreateProgram");
    _LoadFunction(_gl.CreateShader, "glCreateShader");
    _LoadFunction(_gl.DeleteFramebuffers, "glDeleteFramebuffers");
    _LoadFunction(_gl.DeleteShader, "glDeleteShader");
    _LoadFunction(_gl.DeleteTextures, "glDeleteTextures");
    _LoadFunction(_gl.Disable, "glDisable");
    _LoadFunction(_gl.DrawArrays, "glDrawArrays");
    _LoadFunction(_gl.Enable, "glEnable");
    _LoadFunction(_gl.FramebufferTexture2D, "glFramebufferTexture2D");
    _LoadFunction(_gl.GenBuffers, "glGenBuffers");
    _LoadFunction(_gl.GenFramebuffers, "glGenFramebuffers");
    _LoadFunction(_gl.GenSamplers, "glGenSamplers");
    _LoadFunction(_gl.GenTextures, "glGenTextures");
    _LoadFunction(_gl.GenVertexArrays, "glGenVertexArrays");
    _LoadFunction(_gl.GetIntegerv, "glGetIntegerv");
    _LoadFunction(_gl.GetProgramInfoLog, "glGetProgramInfoLog");
    _LoadFunction(_gl.GetProgramiv, "glGetProgramiv");
    _LoadFunction(_gl.GetShaderInfoLog, "glGetShaderInfoLog");
    _LoadFunction(_gl.GetShaderiv, "glGetShaderiv");
    _LoadFunction(_gl.GetUniformBlockIndex, "glGetUniformBlockIndex");
    _LoadFunction(_gl.GetUniformLocation, "glGetUniformLocation");
    _LoadFunction(_gl.LinkProgram, "glLinkProgram");
    _LoadFunction(_gl.PixelStorei, "glPixelStorei");
    _LoadFunction(_gl.ReadPixels, "glReadPixels");
    _LoadFunction(_gl.SamplerParameteri, "glSamplerParameteri");
    _LoadFunction(_gl.Scissor, "glScissor");
    _LoadFunction(_gl.ShaderSource, "glShaderSource");
    _LoadFunction(_gl.TexParameteri, "glTexParameteri");
    _LoadFunction(_gl.TexStorage2D, "glTexStorage2D");
    _LoadFunction(_gl.TexSubImage2D, "glTexSubImage2D");
    _LoadFunction(_gl.Uniform1i, "glUniform1i");
    _LoadFunction(_gl.UniformBlockBinding, "glUniformBlockBinding");
    _LoadFunction(_gl.UseProgram, "glUseProgram");
    _LoadFunction(_gl.Viewport, "glViewport");
}

/* The attributes are set before any window exists: SDL picks a window's GL
   configuration when it creates the window. The context is owned by a hidden
   window of its own, so it outlives every window it draws into; at 640x480
   rather than smaller, because KMSDRM picks its display mode from the size of
   the first window. */
static _Context& _GetContext() {
    PROFILE

    static auto _context = [] {
        App::__InitVideo();

        ::SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        ::SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        ::SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        ::SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        ::SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
        ::SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

        auto context = _Context{};

        context.window = ::SDL_CreateWindow("", 640, 480, (SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_NOT_FOCUSABLE));

        if (context.window == nullptr) {
            throw std::runtime_error(::SDL_GetError());
        }

        context.context = ::SDL_GL_CreateContext(context.window);

        if (context.context == nullptr) {
            auto const error = std::string(::SDL_GetError());
            ::SDL_DestroyWindow(context.window);
            throw std::runtime_error(error);
        }

        _LoadFunctions();

        auto alignment = GLint{ 0 };
        _gl.GetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
        context.uniformAlignment = static_cast<std::size_t>(std::max(alignment, 16));

        _gl.GenVertexArrays(1, &context.vertexArray);
        _gl.BindVertexArray(context.vertexArray); /* the quad's vertices come from a uniform block, but ES 3.0 draws need a vertex array */
        _gl.GenBuffers(1, &context.uniformBuffer);
        _gl.Disable(GL_DITHER);
        _gl.Enable(GL_BLEND);
        _gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
        _gl.PixelStorei(GL_PACK_ALIGNMENT, 1);

        return context;
    }();

    return _context;
}

/* Destroying a window leaves no context current when it was drawn into last,
   and nothing else may have made another current: every entry point checks. */
static _Context& _MakeCurrent() {
    PROFILE

    auto& context = _GetContext();

    if (::SDL_GL_GetCurrentContext() != context.context) {
        if (::SDL_GL_MakeCurrent(context.window, context.context) == false) {
            throw std::runtime_error(::SDL_GetError());
        }
    }

    return context;
}

static GLuint _GetFramebuffer(_Texture& texture) {
    PROFILE

    if (texture.framebuffer == 0) {
        _gl.GenFramebuffers(1, &texture.framebuffer);
        _gl.BindFramebuffer(GL_FRAMEBUFFER, texture.framebuffer);
        _gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.texture, 0);

        if (_gl.CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("Failed to create a framebuffer for a texture");
        }
    }

    return texture.framebuffer;
}

static void _DeleteTexture(_Texture* texture) {
    PROFILE

    _MakeCurrent();

    if (texture->framebuffer != 0) {
        _gl.DeleteFramebuffers(1, &texture->framebuffer);
    }

    _gl.DeleteTextures(1, &texture->texture);

    delete texture;
}

static void _BeginRecording() {
    PROFILE

    _recordingPainters++;
}

static void _EndRecording() {
    PROFILE

    assert(_recordingPainters > 0);

    _recordingPainters--;

    if (_recordingPainters == 0) {
        auto const released = std::exchange(_releasedTextures, {});
        for (auto texture : released) {
            _DeleteTexture(texture);
        }
    }
}

static GLuint _CompileShader(GLenum stage, unsigned char const* code, unsigned int size) {
    PROFILE

    static auto constexpr _HEADER = "#version 300 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2D;\n";

    auto const prefix = (stage == GL_VERTEX_SHADER) ? "#define QUAD_VERTEX\n" : "";

    char const* const strings[] = {
        _HEADER,
        prefix,
        reinterpret_cast<char const*>(Rocket_GLES_Painter),
        reinterpret_cast<char const*>(code)
    };
    GLint const lengths[] = {
        -1,
        -1,
        static_cast<GLint>(Rocket_GLES_Painter_len),
        static_cast<GLint>(size)
    };

    auto shader = _gl.CreateShader(stage);
    _gl.ShaderSource(shader, 4, strings, lengths);
    _gl.CompileShader(shader);

    auto compiled = GLint{ GL_FALSE };
    _gl.GetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (compiled == GL_FALSE) {
        auto log = std::string(1024, '\0');
        auto length = GLsizei{ 0 };
        _gl.GetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), &length, log.data());
        log.resize(static_cast<std::size_t>(length));
        _gl.DeleteShader(shader);
        throw std::runtime_error("Failed to compile a painter shader: " + log);
    }

    return shader;
}

static GLuint _GetVertexShader() {
    PROFILE

    static auto const _shader = _CompileShader(GL_VERTEX_SHADER, Rocket_GLES_QuadVertex, Rocket_GLES_QuadVertex_len);

    return _shader;
}

static GLuint _CreateProgram(unsigned char const* code, unsigned int size) {
    PROFILE

    auto fragment = _CompileShader(GL_FRAGMENT_SHADER, code, size);
    auto program = _gl.CreateProgram();

    _gl.AttachShader(program, _GetVertexShader());
    _gl.AttachShader(program, fragment);
    _gl.LinkProgram(program);
    _gl.DeleteShader(fragment); /* deleted once the program goes */

    auto linked = GLint{ GL_FALSE };
    _gl.GetProgramiv(program, GL_LINK_STATUS, &linked);

    if (linked == GL_FALSE) {
        auto log = std::string(1024, '\0');
        auto length = GLsizei{ 0 };
        _gl.GetProgramInfoLog(program, static_cast<GLsizei>(log.size()), &length, log.data());
        log.resize(static_cast<std::size_t>(length));
        throw std::runtime_error("Failed to link a painter shader: " + log);
    }

    /* GLSL ES 3.00 has no binding layouts: blocks and the sampler are bound by
       name, every brush and filter block at the brush binding. */
    static auto constexpr _BLOCKS = std::array<std::tuple<char const*, GLuint>, 8>{{
        { "StateUniforms",             _STATE_UNIFORMS_BINDING },
        { "ShapeUniforms",             _SHAPE_UNIFORMS_BINDING },
        { "ColorBrushUniforms",        _BRUSH_UNIFORMS_BINDING },
        { "ImageBrushUniforms",        _BRUSH_UNIFORMS_BINDING },
        { "GradientBrushUniforms",     _BRUSH_UNIFORMS_BINDING },
        { "BlurFilterUniforms",        _BRUSH_UNIFORMS_BINDING },
        { "ShadowFilterFieldUniforms", _BRUSH_UNIFORMS_BINDING },
        { "ShadowFilterUniforms",      _BRUSH_UNIFORMS_BINDING }
    }};

    for (auto const& [ name, binding ] : _BLOCKS) {
        auto const index = _gl.GetUniformBlockIndex(program, name);
        if (index != GL_INVALID_INDEX) {
            _gl.UniformBlockBinding(program, index, binding);
        }
    }

    auto const sampler = _gl.GetUniformLocation(program, "texture0");

    if (sampler >= 0) {
        _gl.UseProgram(program);
        _gl.Uniform1i(sampler, 0);
    }

    return program;
}

static GLuint _GetProgram(_Fragment fragment) {
    PROFILE

    static auto _programs = std::map<_Fragment, GLuint>();

    auto it = _programs.find(fragment);
    if (it != _programs.end()) {
        return it->second;
    }

    auto program = GLuint{ 0 };

    switch (fragment) {
        case _Fragment::ColorBrush:            program = _CreateProgram(Rocket_GLES_ColorBrush, Rocket_GLES_ColorBrush_len); break;
        case _Fragment::ImageBrush:            program = _CreateProgram(Rocket_GLES_ImageBrush, Rocket_GLES_ImageBrush_len); break;
        case _Fragment::LinearGradientBrush:   program = _CreateProgram(Rocket_GLES_LinearGradientBrush, Rocket_GLES_LinearGradientBrush_len); break;
        case _Fragment::RadialGradientBrush:   program = _CreateProgram(Rocket_GLES_RadialGradientBrush, Rocket_GLES_RadialGradientBrush_len); break;
        case _Fragment::BlurFilter:            program = _CreateProgram(Rocket_GLES_BlurFilter, Rocket_GLES_BlurFilter_len); break;
        case _Fragment::ShadowFilterDilate:    program = _CreateProgram(Rocket_GLES_ShadowFilterDilate, Rocket_GLES_ShadowFilterDilate_len); break;
        case _Fragment::ShadowFilterBlur:      program = _CreateProgram(Rocket_GLES_ShadowFilterBlur, Rocket_GLES_ShadowFilterBlur_len); break;
        case _Fragment::ShadowFilterComposite: program = _CreateProgram(Rocket_GLES_ShadowFilterComposite, Rocket_GLES_ShadowFilterComposite_len); break;
    }

    _programs.emplace(fragment, program);

    return program;
}

static GLuint _CreateSampler(GLint minFilter, GLint magFilter) {
    PROFILE

    auto sampler = GLuint{ 0 };

    _gl.GenSamplers(1, &sampler);
    _gl.SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, minFilter);
    _gl.SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, magFilter);
    _gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    _gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    _gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return sampler;
}

static GLuint _GetLinLinSampler() {
    PROFILE

    static auto const _sampler = _CreateSampler(GL_LINEAR, GL_LINEAR);

    return _sampler;
}

static GLuint _GetNerNerSampler() {
    PROFILE

    static auto const _sampler = _CreateSampler(GL_NEAREST, GL_NEAREST);

    return _sampler;
}

static GLuint _GetLinNerSampler() {
    PROFILE

    static auto const _sampler = _CreateSampler(GL_LINEAR, GL_NEAREST);

    return _sampler;
}

static GLuint _GetNerLinSampler() {
    PROFILE

    static auto const _sampler = _CreateSampler(GL_NEAREST, GL_LINEAR);

    return _sampler;
}

static int _GetGaussianTaps(float extent) {
    return std::clamp(static_cast<int>(std::ceil(extent)), 0, _BLUR_TAP_MAX);
}

static void _CopyColumn(float* column, float x, float y, float z) {
    column[0] = x;
    column[1] = y;
    column[2] = z;
    column[3] = 0.0f;
}

/* Pixel rows run top down on both kinds of target, but an image's first row
   is GL's bottom one, so only a window's projection flips Y. */
static _StateUniforms _MakeStateUniforms(_Target const& target, Mat3 const& transform, float opacity) {
    PROFILE

    auto const resolution = Vec2{
        static_cast<float>(target.width),
        static_cast<float>(target.height)
    };
    auto const flip = (target.texture == nullptr) ? -1.0f : 1.0f;

    auto stateUniforms = _StateUniforms{};
    stateUniforms.resolution[0] = resolution.x;
    stateUniforms.resolution[1] = resolution.y;
    _CopyColumn(&stateUniforms.projection[0], (2.0f / resolution.width), 0.0f, 0.0f);
    _CopyColumn(&stateUniforms.projection[4], 0.0f, ((2.0f * flip) / resolution.height), 0.0f);
    _CopyColumn(&stateUniforms.projection[8], -1.0f, -flip, 1.0f);
    _CopyColumn(&stateUniforms.transform[0], transform.data[0], transform.data[1], transform.data[2]);
    _CopyColumn(&stateUniforms.transform[4], transform.data[3], transform.data[4], transform.data[5]);
    _CopyColumn(&stateUniforms.transform[8], transform.data[6], transform.data[7], transform.data[8]);
    stateUniforms.opacity = opacity;

    return stateUniforms;
}

static _ShapeUniforms _MakeShapeUniforms(Shape const& shape, Vec2 const& expand) {
    PROFILE

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

    return _ShapeUniforms{
        .vertices = {
            { (rect.x - expand.x),         (rect.y - expand.y),         0.0f, 0.0f },
            { (rect.x - expand.x),         (rect.getMaxY() + expand.y), 0.0f, 0.0f },
            { (rect.getMaxX() + expand.x), (rect.y - expand.y),         0.0f, 0.0f },
            { (rect.getMaxX() + expand.x), (rect.getMaxY() + expand.y), 0.0f, 0.0f }
        },
        .bounds  = { rect.x, rect.y, rect.width, rect.height }, // unexpanded: localPos and localUV stay in shape space
        .borders = { borders.left, borders.top, borders.right, borders.bottom },
        .corners = { corners.data[0], corners.data[1], corners.data[2], corners.data[3] },
        .type    = type,
        ._pad0   = {}
    };
}

static _ColorBrushUniforms _MakeColorBrushUniforms(ColorBrush const& colorBrush) {
    PROFILE

    return _ColorBrushUniforms{
        .color = { colorBrush.color.red, colorBrush.color.green, colorBrush.color.blue, colorBrush.color.alpha }
    };
}

static _ImageBrushUniforms _MakeImageBrushUniforms(ImageBrush const& imageBrush, Vec4 const& bounds) {
    PROFILE

    assert(imageBrush.image != nullptr);

    auto const imageSize = imageBrush.image->getSize();
    auto const imageColor = imageBrush.color.value_or(Vec4{});
    auto const imageNPatch = imageBrush.nPatch.value_or(Vec4{});
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

    return _ImageBrushUniforms{
        .color  = { imageColor.red, imageColor.green, imageColor.blue, imageColor.alpha },
        .size   = { imageSize.width, imageSize.height },
        .flip   = { (flipX ? 1.0f : 0.0f), (flipY ? 1.0f : 0.0f) },
        .repeat = { (repeatX ? 1.0f : 0.0f), (repeatY ? 1.0f : 0.0f) },
        ._pad0  = {},
        .npatch = { imageNPatch.left, imageNPatch.top, imageNPatch.right, imageNPatch.bottom },
        .source = { imageSrc.x, imageSrc.y, imageSrc.getMaxX(), imageSrc.getMaxY() },
        .destin = { imageDst.x, imageDst.y, imageDst.getMaxX(), imageDst.getMaxY() }
    };
}

static GLuint _GetImageSampler(ImageBrush const& imageBrush) {
    PROFILE

    auto const filterMin = imageBrush.filterMin.value_or(ImageFilter::Nearest);
    auto const filterMag = imageBrush.filterMag.value_or(ImageFilter::Nearest);

    if (filterMin == ImageFilter::Linear && filterMag == ImageFilter::Linear) {
        return _GetLinLinSampler();
    } else if (filterMin == ImageFilter::Linear && filterMag == ImageFilter::Nearest) {
        return _GetLinNerSampler();
    } else if (filterMin == ImageFilter::Nearest && filterMag == ImageFilter::Linear) {
        return _GetNerLinSampler();
    }

    return _GetNerNerSampler();
}

static _GradientBrushUniforms _MakeGradientBrushUniforms(GradientBrush const& gradientBrush) {
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

    return gradientUniforms;
}

static _BlurFilterUniforms _MakeBlurFilterUniforms(Vec2 const& texelSize, Vec2 const& direction, BlurFilter const& filter, Vec2 const& scale) {
    PROFILE

    auto const axisScale = ((direction.x != 0.0f) ? scale.x : scale.y);

    return _BlurFilterUniforms{
        .texelSize = { texelSize.x, texelSize.y },
        .direction = { direction.x, direction.y },
        .sigma     = (filter.getSigma() * axisScale),
        .taps      = _GetGaussianTaps(filter.getExtent() * axisScale),
        ._pad0     = {}
    };
}

static _ShadowFilterFieldUniforms _MakeShadowFilterFieldUniforms(Vec2 const& texel, Vec2 const& direction, float spread, ShadowFilter const& filter, bool fromSilhouette, Vec2 const& scale) {
    PROFILE

    auto const axisScale = ((direction.x != 0.0f) ? scale.x : scale.y);

    return _ShadowFilterFieldUniforms{
        .texel          = { texel.x, texel.y },
        .direction      = { direction.x, direction.y },
        .spread         = std::clamp((spread * axisScale), -static_cast<float>(_BLUR_TAP_MAX), static_cast<float>(_BLUR_TAP_MAX)),
        .sigma          = (filter.getSigma() * axisScale),
        .taps           = _GetGaussianTaps(filter.getExtent() * axisScale),
        .fromSilhouette = (fromSilhouette ? 1 : 0),
        .invert         = ((fromSilhouette && filter.inset) ? 1 : 0),
        ._pad0          = {}
    };
}

static _ShadowFilterUniforms _MakeShadowFilterUniforms(Vec2 const& texel, ShadowFilter const& filter, bool fromSilhouette, Vec2 const& scale) {
    PROFILE

    auto const offset = filter.offset.value_or(Vec2{});

    return _ShadowFilterUniforms{
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
}

/* Binds the target a command draws into; false when it is a window that has
   been destroyed since the command was recorded. */
static bool _BindTarget(_Context const& context, _Target const& target) {
    PROFILE

    if (target.texture != nullptr) {
        _gl.BindFramebuffer(GL_FRAMEBUFFER, _GetFramebuffer(*target.texture));
    } else {
        auto window = ::SDL_GetWindowFromID(target.window);

        if ((window == nullptr) || (::SDL_GL_MakeCurrent(window, context.context) == false)) {
            return false;
        }

        _gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    _gl.Viewport(0, 0, static_cast<GLsizei>(target.width), static_cast<GLsizei>(target.height));

    return true;
}

static void _RunDraw(_Context const& context, _Draw const& draw) {
    PROFILE

    _gl.UseProgram(_GetProgram(draw.fragment));
    _SetBlend(draw.blend);
    _gl.Enable(GL_SCISSOR_TEST);
    _gl.Scissor(draw.scissor[0], draw.scissor[1], draw.scissor[2], draw.scissor[3]);
    _gl.BindBufferRange(GL_UNIFORM_BUFFER, _STATE_UNIFORMS_BINDING, context.uniformBuffer, static_cast<GLintptr>(draw.state.offset), static_cast<GLsizeiptr>(draw.state.size));
    _gl.BindBufferRange(GL_UNIFORM_BUFFER, _SHAPE_UNIFORMS_BINDING, context.uniformBuffer, static_cast<GLintptr>(draw.shape.offset), static_cast<GLsizeiptr>(draw.shape.size));
    _gl.BindBufferRange(GL_UNIFORM_BUFFER, _BRUSH_UNIFORMS_BINDING, context.uniformBuffer, static_cast<GLintptr>(draw.brush.offset), static_cast<GLsizeiptr>(draw.brush.size));

    if (draw.texture != nullptr) {
        _gl.ActiveTexture(GL_TEXTURE0);
        _gl.BindTexture(GL_TEXTURE_2D, draw.texture->texture);
        _gl.BindSampler(0, draw.sampler);
    }

    _gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void _RunClear(_Clear const& clear) {
    PROFILE

    _gl.Disable(GL_SCISSOR_TEST);
    _gl.ClearColor(clear.color[0], clear.color[1], clear.color[2], clear.color[3]);
    _gl.Clear(GL_COLOR_BUFFER_BIT);
}

/* A window is presented with vsync, set on its own surface the first time it is drawn into. */
static void _Present(SDL_Window* window) {
    PROFILE

    static auto _synced = std::vector<SDL_WindowID>();

    auto const id = ::SDL_GetWindowID(window);

    if (std::ranges::find(_synced, id) == _synced.end()) {
        ::SDL_GL_SetSwapInterval(1);
        _synced.push_back(id);
    }

    ::SDL_GL_SwapWindow(window);
}

void* __GetDefaultGPUDevice() {
    PROFILE

    return _GetContext().context;
}

int __GetDefaultTextureFormat() {
    PROFILE

    return GL_RGBA8;
}

std::uint64_t __GetWindowFlags() {
    PROFILE

    _GetContext();

    return SDL_WINDOW_OPENGL;
}

bool __ClaimWindow(SDL_Window*) {
    PROFILE

    return true;
}

void __ReleaseWindow(SDL_Window* window) {
    PROFILE

    auto& context = _GetContext();

    if (::SDL_GL_GetCurrentWindow() == window) {
        ::SDL_GL_MakeCurrent(context.window, context.context);
    }
}

void* __CreateTexture(std::uint32_t width, std::uint32_t height) {
    PROFILE

    assert(width > 0);
    assert(height > 0);

    _MakeCurrent();

    auto texture = new _Texture{ 0, 0, width, height };

    _gl.GenTextures(1, &texture->texture);
    _gl.BindTexture(GL_TEXTURE_2D, texture->texture);
    _gl.TexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    _gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    /* GL leaves new storage undefined; images start transparent, as on the other backends */
    _gl.BindFramebuffer(GL_FRAMEBUFFER, _GetFramebuffer(*texture));
    _gl.Disable(GL_SCISSOR_TEST);
    _gl.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    _gl.Clear(GL_COLOR_BUFFER_BIT);

    return texture;
}

void __ReleaseTexture(void* texture) {
    PROFILE

    if (texture == nullptr) {
        return;
    }

    if (_recordingPainters > 0) {
        _releasedTextures.push_back(static_cast<_Texture*>(texture));
    } else {
        _DeleteTexture(static_cast<_Texture*>(texture));
    }
}

void __UploadTexture(void* texture, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, std::uint8_t const* pixels) {
    PROFILE

    if ((width == 0) || (height == 0)) {
        return;
    }

    _MakeCurrent();

    _gl.BindTexture(GL_TEXTURE_2D, static_cast<_Texture*>(texture)->texture);
    _gl.TexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(x), static_cast<GLint>(y), static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

void __DownloadTexture(void* texture, std::vector<std::uint8_t>& pixels) {
    PROFILE

    _MakeCurrent();

    auto& source = *static_cast<_Texture*>(texture);

    pixels.resize(static_cast<std::size_t>(source.width) * source.height * 4);

    _gl.BindFramebuffer(GL_FRAMEBUFFER, _GetFramebuffer(source));
    _gl.ReadPixels(0, 0, static_cast<GLsizei>(source.width), static_cast<GLsizei>(source.height), GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
}

void __CopyTexture(void* source, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, void* destination, std::uint32_t toX, std::uint32_t toY) {
    PROFILE

    if ((width == 0) || (height == 0)) {
        return;
    }

    _MakeCurrent();

    auto const from = _GetFramebuffer(*static_cast<_Texture*>(source));
    auto const to = _GetFramebuffer(*static_cast<_Texture*>(destination));

    _gl.Disable(GL_SCISSOR_TEST); /* the scissor clips blits too */
    _gl.BindFramebuffer(GL_READ_FRAMEBUFFER, from);
    _gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, to);
    _gl.BlitFramebuffer(
        static_cast<GLint>(x), static_cast<GLint>(y), static_cast<GLint>(x + width), static_cast<GLint>(y + height),
        static_cast<GLint>(toX), static_cast<GLint>(toY), static_cast<GLint>(toX + width), static_cast<GLint>(toY + height),
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );
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

template <typename Uniforms>
_UniformRange Painter::_Painter::pushUniforms(Uniforms const& data) {
    PROFILE

    auto const alignment = _GetContext().uniformAlignment;
    auto const offset = (((uniforms.size() + alignment - 1) / alignment) * alignment);

    uniforms.resize(offset + sizeof(Uniforms));
    std::memcpy(uniforms.data() + offset, &data, sizeof(Uniforms));

    return _UniformRange{ offset, sizeof(Uniforms) };
}

void Painter::_Painter::replay() {
    PROFILE

    auto& context = _MakeCurrent();

    if (commands.empty() == false) {
        _gl.BindBuffer(GL_UNIFORM_BUFFER, context.uniformBuffer);
        _gl.BufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(uniforms.size()), uniforms.data(), GL_STREAM_DRAW);

        auto bound = std::optional<_Target>();
        auto skipped = false;

        for (auto const& command : commands) {
            auto const& target = std::visit([](auto const& value) -> _Target const& { return value.target; }, command);

            if (bound != target) {
                bound = target;
                skipped = (_BindTarget(context, target) == false);
            }

            if (skipped) {
                continue;
            }

            if (auto const draw = std::get_if<_Draw>(&command)) {
                _RunDraw(context, *draw);
            } else if (auto const clear = std::get_if<_Clear>(&command)) {
                _RunClear(*clear);
            }
        }
    }

    for (auto const id : windows) {
        auto window = ::SDL_GetWindowFromID(id);
        if ((window != nullptr) && ::SDL_GL_MakeCurrent(window, context.context)) {
            _Present(window);
        }
    }

    drop();
}

void Painter::_Painter::drop() {
    PROFILE

    commands.clear();
    uniforms.clear();
    windows.clear();
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

    /* The open passes are abandoned: their commands are dropped, so every
       target keeps its previous content. A window drawn into is the exception,
       as on the SDL3 painter: everything recorded is run, and the window shows
       what was painted so far. */
    if (_impl->passStack.empty() == false) {
        if (_impl->windows.empty() == false) {
            _impl->replay();
        } else {
            _impl->drop();
        }

        while (_impl->passStack.empty() == false) {
            _impl->passStack.pop();
        }

        _EndRecording();
    }

    for (auto& [ key, filterTextures ] : _impl->filterTextures) {
        __ReleaseTexture(std::get<0>(filterTextures.textures));
        __ReleaseTexture(std::get<1>(filterTextures.textures));
    }

    delete _impl;
    _impl = nullptr;
}

void Painter::__beginPaint(PaintTarget const& target) {
    PROFILE

    if (_impl->passStack.empty()) {
        _BeginRecording();
    }

    auto const premultiply = [](Vec4 const& color) {
        return std::array<float, 4>{ /* the target is premultiplied; the clear color is straight RGBA */
            (color.red * color.alpha),
            (color.green * color.alpha),
            (color.blue * color.alpha),
            color.alpha
        };
    };

    target.match(
        [&](ImagePaintTarget const& imageTarget) {
            assert(imageTarget.image.getTexture() != nullptr);

            auto const pass = _Target{
                .texture = static_cast<_Texture*>(imageTarget.image.getTexture()),
                .window = 0,
                .width = (std::uint32_t)imageTarget.image.getSize().width,
                .height = (std::uint32_t)imageTarget.image.getSize().height
            };

            _impl->passStack.emplace(pass);

            if (imageTarget.clearColor.has_value()) {
                _impl->commands.emplace_back(_Clear{ pass, premultiply(*imageTarget.clearColor) });
            }
        },
        [&](WindowPaintTarget const& windowTarget) {
            assert(windowTarget.window.getHandle() != nullptr);

            auto window = static_cast<SDL_Window*>(windowTarget.window.getHandle());
            auto width = 0;
            auto height = 0;

            if ((::SDL_GetWindowSizeInPixels(window, &width, &height) == false) || (width <= 0) || (height <= 0)) {
                _impl->passStack.emplace(); /* nothing to draw into (a minimized window): the pass is a no-op */
                return;
            }

            auto const pass = _Target{
                .texture = nullptr,
                .window = ::SDL_GetWindowID(window),
                .width = static_cast<std::uint32_t>(width),
                .height = static_cast<std::uint32_t>(height)
            };

            _impl->passStack.emplace(pass);

            if (std::ranges::find(_impl->windows, pass.window) == _impl->windows.end()) {
                _impl->windows.push_back(pass.window);
            }

            if (windowTarget.clearColor.has_value()) {
                _impl->commands.emplace_back(_Clear{ pass, premultiply(*windowTarget.clearColor) });
            }
        }
    );
}

void Painter::__endPaint() {
    PROFILE

    if (_impl->passStack.empty()) {
        return;
    }

    _impl->passStack.pop();

    if (_impl->passStack.empty()) {
        _impl->replay();
        _impl->evictFilterTextures();
        _EndRecording();
    }
}

void Painter::__paint(Shape const& shape, Brush const& brush, PaintOptions const& options, Vec4 const& bounds) const {
    PROFILE

    if (_impl->passStack.empty()) {
        return;
    }

    if (_impl->passStack.top().has_value() == false) {
        return;
    }

    auto const renderPass = *_impl->passStack.top();

    auto const colorBrush = brush.as<ColorBrush>();
    auto const imageBrush = brush.as<ImageBrush>();
    auto const gradientBrush = brush.as<GradientBrush>();
    auto const shadowFilter = options.filter.has_value() ? options.filter->as<ShadowFilter>() : nullptr;
    auto const blurFilter = options.filter.has_value() ? options.filter->as<BlurFilter>() : nullptr;
    auto const blend = options.blend.value_or(Blend::Over);

    auto const record = [&](_Target const& pass, _Fragment fragment, Blend mode, std::optional<Vec4> const& scissor, _Texture* texture, GLuint sampler, Mat3 const& transform, float opacity, Shape const& quad, Vec2 const& expand, _UniformRange const& brushUniforms) {
        _impl->commands.emplace_back(_Draw{
            .target   = pass,
            .fragment = fragment,
            .blend    = mode,
            .scissor  = _GetScissor(pass, scissor),
            .texture  = texture,
            .sampler  = sampler,
            .state    = _impl->pushUniforms(_MakeStateUniforms(pass, transform, opacity)),
            .shape    = _impl->pushUniforms(_MakeShapeUniforms(quad, expand)),
            .brush    = brushUniforms
        });
    };

    auto const drawShape = [&](_Target const& pass, std::optional<Vec4> const& scissor, Blend mode, Vec2 const& scale) {
        auto const transform = options.transform.value_or(Mat3{}).toScaled(scale);
        auto fragment = _Fragment::ColorBrush;
        auto brushUniforms = _UniformRange{};
        auto texture = static_cast<_Texture*>(nullptr);
        auto sampler = GLuint{ 0 };

        if (colorBrush != nullptr) {
            fragment = _Fragment::ColorBrush;
            brushUniforms = _impl->pushUniforms(_MakeColorBrushUniforms(*colorBrush));
        } else if (imageBrush != nullptr) {
            fragment = _Fragment::ImageBrush;
            brushUniforms = _impl->pushUniforms(_MakeImageBrushUniforms(*imageBrush, bounds));
            texture = static_cast<_Texture*>(imageBrush->image->getTexture());
            sampler = _GetImageSampler(*imageBrush);
        } else if (gradientBrush != nullptr) {
            fragment = gradientBrush->radial.value_or(false) ? _Fragment::RadialGradientBrush : _Fragment::LinearGradientBrush;
            brushUniforms = _impl->pushUniforms(_MakeGradientBrushUniforms(*gradientBrush));
        }

        record(pass, fragment, mode, scissor, texture, sampler, transform, options.opacity.value_or(1.0f), shape, _EdgeExpansion(transform), brushUniforms);
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
    auto const scratch = [&](_Texture* texture) {
        auto const pass = _Target{ .texture = texture, .window = 0, .width = filterWidth, .height = filterHeight };
        _impl->commands.emplace_back(_Clear{ pass, { 0.0f, 0.0f, 0.0f, 0.0f } });
        return pass;
    };

    auto const drawBlur = [&](_Target const& pass, _Texture* source, Vec2 const& direction, Vec4 const& rect, std::optional<Vec4> const& scissor, Blend mode) {
        assert(source != nullptr);

        auto const blurUniforms = _impl->pushUniforms(_MakeBlurFilterUniforms(filterTexel, direction, *blurFilter, filterScale));

        record(pass, _Fragment::BlurFilter, mode, scissor, source, _GetLinLinSampler(), Mat3{}, 1.0f, Shape{ QuadShape{ rect } }, Vec2{}, blurUniforms);
    };

    auto const drawShadowField = [&](_Target const& pass, _Texture* source, _Fragment fragment, Vec2 const& direction, float spread, bool fromSilhouette, Vec4 const& rect) {
        assert(source != nullptr);

        auto const fieldUniforms = _impl->pushUniforms(_MakeShadowFilterFieldUniforms(filterTexel, direction, spread, *shadowFilter, fromSilhouette, filterScale));

        record(pass, fragment, Blend::Over, rect, source, _GetLinLinSampler(), Mat3{}, 1.0f, Shape{ QuadShape{ rect } }, Vec2{}, fieldUniforms);
    };

    auto const drawShadowComposite = [&](_Target const& pass, _Texture* source, bool fromSilhouette, Vec4 const& rect, std::optional<Vec4> const& scissor, Blend mode) {
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
        auto const shadowUniforms = _impl->pushUniforms(_MakeShadowFilterUniforms(filterTexel, *shadowFilter, fromSilhouette, filterScale));

        record(pass, _Fragment::ShadowFilterComposite, mode, scissor, source, _GetLinLinSampler(), transform, 1.0f, Shape{ QuadShape{ quad } }, Vec2{}, shadowUniforms);
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

        auto const& [ shapeTexture, blurTexture ] = _impl->getFilterTextures(filterWidth, filterHeight).textures;

        drawShape(scratch(shapeTexture), std::nullopt, Blend::Over, filterScale);
        drawBlur(scratch(blurTexture), shapeTexture, Vec2{ 1.0f, 0.0f }, scratchRect, scratchRect, Blend::Over);
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

        auto [ source, target ] = _impl->getFilterTextures(filterWidth, filterHeight).textures;
        auto const spread = shadowFilter->spread.value_or(Vec2{});
        auto fromSilhouette = true;

        drawShape(scratch(source), std::nullopt, Blend::Over, filterScale);

        auto const fieldPass = [&](_Fragment fragment, Vec2 const& direction, float extent) {
            drawShadowField(scratch(target), source, fragment, direction, extent, fromSilhouette, scratchRect);
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

        drawShadowComposite(renderPass, source, fromSilhouette, *filterBounds, targetScissor, blend);
    }
}

} /* namespace Rocket */
