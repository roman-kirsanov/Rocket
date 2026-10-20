/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Shared by the Win32 painter shaders: the constant buffers (mirrored by the
// structs in Painter_Win32.cpp) and the coverage helpers. The shaders are
// those of Paint/GLSL, line for line. Registers follow the root signature:
// vertex constants in space 1, the pixel texture and sampler in space 2 and
// pixel constants in space 3, at the slots the Metal painter uses.
//
// HLSL packs these structs as the GLSL blocks are packed (std140), but for
// two cases spelled out as padding: std140 aligns a float2 to 8 bytes, where
// HLSL packs it right after a scalar, and std140 rounds a matrix up to whole
// registers, where HLSL packs a scalar into a float3x3's last spare float.

struct StateUniforms {
    float2   resolution;
    float3x3 projection;     // column-major: one register per column, as a GLSL mat3
    float3x3 transform;
    float    _pad0;          // std140 rounds a mat3 up to three whole registers; HLSL would put opacity in the last column's spare float
    float    opacity;
};

// vertices are float4 for the array stride; only .xy is used.
struct ShapeUniforms {
    float4 vertices[4];
    float4 bounds;
    float4 borders; // per-edge outline widths (left, top, right, bottom)
    float4 corners; // per-corner radii (top-left, top-right, bottom-right, bottom-left)
    int    type;    // 0=quad, 1=ellipse, 2=quad outline, 3=ellipse outline
};

// Colors are premultiplied. stopPositions packs the 10 positions four to a
// float4 for the array stride: position i is stopPositions[i / 4][i % 4].
struct GradientBrushUniforms {
    float2 startPoint;
    float4 startColor;
    float2 stopPoint;
    float4 stopColor;
    int    stopCount;
    float4 stopColors[10];
    float4 stopPositions[3];
};

struct ShadowFilterFieldUniforms {
    float2 texel;
    float2 direction;      // (1, 0) horizontal pass, (0, 1) vertical pass
    float  spread;         // dilate pass: extent in pixels along direction; negative erodes
    float  sigma;          // blur pass: gaussian standard deviation in texels
    int    taps;           // blur pass: gaussian half-width in taps
    int    fromSilhouette; // 1 = source is the shape texture (.a), 0 = a field texture (.r/.g)
    int    invert;         // 1 = complement the field (first pass of an inset chain)
};

// Interpolated outputs of QuadVertex.hlsl, the same for every pixel shader so
// the stages' signatures always line up.
struct QuadVaryings {
    float4 position  : SV_Position;
    float2 localPos  : TEXCOORD0;
    float2 localUV   : TEXCOORD1;
    float2 screenUV  : TEXCOORD2;
};

// The helpers below are pixel-stage only (derivatives, samplers); the vertex
// shader defines QUAD_VERTEX to leave them out.
#ifndef QUAD_VERTEX

// The derivatives are taken per pixel, as on Metal, so the antialiasing ramp
// is the same width on every backend.
float _Fwidth(float value) {
    return abs(ddx_fine(value)) + abs(ddy_fine(value));
}

// One axis of the nine-patch remap: dst offset p in [0, dstSize) to the source
// offset inside the slice. Bands are [0, dstA), [dstA, dstSize - dstB) and
// [dstSize - dstB, dstSize); each stretches linearly onto its source band. A
// band is entered only when its dst width is positive (dstFit keeps dstA + dstB
// within dstSize), so every divisor below is nonzero.
float _npatchAxis(float p, float srcSize, float dstSize, float srcA, float srcB, float dstA, float dstB) {
    if (p < dstA) {
        return (srcA * (p / dstA));
    }
    float srcEnd = (srcSize - srcB);
    float dstEnd = (dstSize - dstB);
    if (p < dstEnd) {
        return (srcA + ((srcEnd - srcA) * ((p - dstA) / (dstEnd - dstA))));
    }
    return (srcEnd + ((srcSize - srcEnd) * ((p - dstEnd) / (dstSize - dstEnd))));
}

// Signed distance (pixels, negative inside) from a rounded box whose top-left
// corner sits at the local origin (Y down). corners: x=TL, y=TR, z=BR, w=BL.
// Each in-region corner constraint is max-combined with the plain box SDF; see
// Painter.metal for why.
float _roundBoxDistance(float2 localPos, float2 size, float4 corners) {
    float2 edges = max(-localPos, (localPos - size));
    float distance = (length(max(edges, 0.0)) + min(max(edges.x, edges.y), 0.0));
    if ((localPos.x < corners.x) && (localPos.y < corners.x)) {
        distance = max(distance, (length(localPos - float2(corners.x, corners.x)) - corners.x));
    }
    if ((localPos.x > (size.x - corners.y)) && (localPos.y < corners.y)) {
        distance = max(distance, (length(localPos - float2((size.x - corners.y), corners.y)) - corners.y));
    }
    if ((localPos.x > (size.x - corners.z)) && (localPos.y > (size.y - corners.z))) {
        distance = max(distance, (length(localPos - float2((size.x - corners.z), (size.y - corners.z))) - corners.z));
    }
    if ((localPos.x < corners.w) && (localPos.y > (size.y - corners.w))) {
        distance = max(distance, (length(localPos - float2(corners.w, (size.y - corners.w))) - corners.w));
    }
    return distance;
}

// Half-pixel linear edge profile of a signed distance (negative inside),
// normalised by its screen-space gradient.
float _edgeCoverage(float d) {
    return clamp((0.5 - (d / max(_Fwidth(d), 0.0001))), 0.0, 1.0);
}

float _quadCoverage(float2 localPos, float2 size, float4 corners) {
    if (any(corners > 0.0)) {
        return _edgeCoverage(_roundBoxDistance(localPos, size, corners));
    }
    float2 edges = max(-localPos, (localPos - size));
    return (_edgeCoverage(edges.x) * _edgeCoverage(edges.y));
}

float _coverage(float2 localPos, float4 bounds, float4 borders, float4 corners, int type) {
    if (type == 1) {
        float2 c = (bounds.zw * 0.5);
        return _edgeCoverage(length((localPos - c) / c) - 1.0);
    } else if (type == 2) {
        float4 b        = borders;
        float  outerCov = _quadCoverage(localPos, bounds.zw, corners);
        if (any(corners > 0.0)) {
            float  innerCov  = 0.0;
            float2 innerSize = (bounds.zw - float2((b.x + b.z), (b.y + b.w)));
            if (all(innerSize > 0.0)) {
                float4 innerCorners = max(float4(
                    (corners.x - max(b.x, b.y)),
                    (corners.y - max(b.z, b.y)),
                    (corners.z - max(b.z, b.w)),
                    (corners.w - max(b.x, b.w))
                ), 0.0);
                innerCov = _edgeCoverage(_roundBoxDistance((localPos - b.xy), innerSize, innerCorners));
            }
            return (outerCov * (1.0 - innerCov));
        }
        float big = (max(bounds.z, bounds.w) + 1.0);
        float dL  = (b.x > 0.0) ? (localPos.x - b.x)              : big;
        float dT  = (b.y > 0.0) ? (localPos.y - b.y)              : big;
        float dR  = (b.z > 0.0) ? ((bounds.z - b.z) - localPos.x) : big;
        float dB  = (b.w > 0.0) ? ((bounds.w - b.w) - localPos.y) : big;
        float holeCov = (_edgeCoverage(-min(dL, dR)) * _edgeCoverage(-min(dT, dB)));
        return (outerCov * (1.0 - holeCov));
    } else if (type == 3) {
        float2 c  = (bounds.zw * 0.5);
        float2 ci = max((c - borders.x), 0.0001);
        float  outerCov = _edgeCoverage(length((localPos - c) / c)  - 1.0);
        float  innerCov = _edgeCoverage(length((localPos - c) / ci) - 1.0);
        return (outerCov * (1.0 - innerCov));
    }
    return _quadCoverage(localPos, bounds.zw, corners);
}

float _gaussianWeight(int i, float sigma) {
    float x = (float)i;
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

// Shadow field (x) and carried silhouette mask (y) at uv; see Painter.metal.
float2 _shadowField(Texture2D<float4> source, SamplerState sampler0, float2 uv, int fromSilhouette, int invert) {
    float4 t = source.Sample(sampler0, uv);
    float2 f = (fromSilhouette != 0) ? t.aa : t.rg;
    f.x = (invert != 0) ? (1.0 - f.x) : f.x;
    return f;
}

#endif
