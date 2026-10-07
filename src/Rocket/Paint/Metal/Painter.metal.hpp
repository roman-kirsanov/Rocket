#pragma once

#ifdef __METAL_VERSION__
    #include <metal_stdlib>

    #define INT1     int
    #define FLOAT1   float
    #define FLOAT2   metal::float2
    #define FLOAT4   metal::float4
    #define FLOAT3X3 metal::float3x3
#else
    #include <simd/simd.h>

    using INT1     = int;
    using FLOAT1   = float;
    using FLOAT2   = simd_float2;
    using FLOAT4   = simd_float4;
    using FLOAT3X3 = simd_float3x3;
#endif

/** Per-pass state shared by every draw: resolution, projection, transform, and opacity. */
struct StateUniforms {
    FLOAT2   resolution;
    FLOAT3X3 projection;
    FLOAT3X3 transform;
    FLOAT1   opacity;
};

/** Geometry of the shape being drawn, shared by the vertex and fragment stages. */
struct ShapeUniforms {
    FLOAT2 vertices[4];
    FLOAT4 bounds;
    FLOAT4 borders; // per-edge outline widths (left, top, right, bottom)
    FLOAT4 corners; // per-corner radii (top-left, top-right, bottom-right, bottom-left)
    INT1   type;    // 0=quad, 1=ellipse, 2=quad outline, 3=ellipse outline
};

/** Uniforms for the ColorBrush fragment shader. */
struct ColorBrushUniforms {
    FLOAT4 color;
};

/** Uniforms for the ImageBrush fragment shader. */
struct ImageBrushUniforms {
    FLOAT4 color;
    FLOAT2 size;
    FLOAT2 flip;
    FLOAT2 repeat;
    FLOAT4 npatch;
    FLOAT4 source;
    FLOAT4 destin;
};

/** Uniforms for the GradientBrush fragment shaders (linear and radial). */
struct GradientBrushUniforms {
    FLOAT2 startPoint;
    FLOAT4 startColor;
    FLOAT2 stopPoint;
    FLOAT4 stopColor;
    INT1   stopCount;
    FLOAT4 stopColors[10];
    FLOAT1 stopPosition[10];
};

/** Uniforms for the BlurFilter two-pass gaussian blur fragment shader. */
struct BlurFilterUniforms {
    FLOAT2 texelSize;
    FLOAT2 direction;
    FLOAT1 sigma;
    INT1   taps;
};

/** Uniforms for the ShadowFilter composite pass. */
struct ShadowFilterUniforms {
    FLOAT4 color;
    FLOAT1 opacity;
    FLOAT2 texel;
    FLOAT2 offset;         // inset field displacement, in pixels
    INT1   radius;
    INT1   inset;          // 0 = drop shadow, 1 = inset
    INT1   fromSilhouette; // 1 = source is the shape texture (.a), 0 = a field texture (.r/.g)
    INT1   invert;         // 1 = complement the field (no earlier pass did it)
};

/**
 * Uniforms for the ShadowFilter separable dilate and blur passes. The shape
 * texture holds the silhouette in .a; every pass after it stores the field in
 * .r and carries the untouched silhouette mask in .g.
 */
struct ShadowFilterFieldUniforms {
    FLOAT2 texel;
    FLOAT2 direction;      // (1, 0) horizontal pass, (0, 1) vertical pass
    FLOAT1 spread;         // dilate pass: extent in pixels along direction; negative erodes
    INT1   radius;         // blur pass: gaussian half-width in taps
    INT1   fromSilhouette; // 1 = source is the shape texture (.a), 0 = a field texture (.r/.g)
    INT1   invert;         // 1 = complement the field (first pass of an inset chain)
};