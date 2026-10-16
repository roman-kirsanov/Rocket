/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Shared by the SDL3 painter shaders: the std140 uniform blocks (mirrored by
// the structs in Painter_SDL3.cpp) and the coverage helpers. Bindings follow
// SDL_CreateGPUShader for SPIR-V: vertex uniforms in set 1, fragment samplers
// in set 2 and fragment uniforms in set 3, at the slots the Metal painter uses.

#define STATE_UNIFORMS                                          \
    vec2  resolution;                                           \
    mat3  projection;                                           \
    mat3  transform;                                            \
    float opacity;

// vertices are vec4 for the std140 array stride; only .xy is used.
#define SHAPE_UNIFORMS                                          \
    vec4 vertices[4];                                           \
    vec4 bounds;                                                \
    vec4 borders; /* per-edge outline widths (left, top, right, bottom) */           \
    vec4 corners; /* per-corner radii (top-left, top-right, bottom-right, bottom-left) */ \
    int  type;    /* 0=quad, 1=ellipse, 2=quad outline, 3=ellipse outline */

// Colors are premultiplied. stopPositions packs the 10 positions four to a
// vec4 for the std140 array stride: position i is stopPositions[i / 4][i % 4].
#define GRADIENT_BRUSH_UNIFORMS                                 \
    vec2  startPoint;                                           \
    vec4  startColor;                                           \
    vec2  stopPoint;                                            \
    vec4  stopColor;                                            \
    int   stopCount;                                            \
    vec4  stopColors[10];                                       \
    vec4  stopPositions[3];

#define SHADOW_FILTER_FIELD_UNIFORMS                            \
    vec2  texel;                                                \
    vec2  direction;      /* (1, 0) horizontal pass, (0, 1) vertical pass */ \
    float spread;         /* dilate pass: extent in pixels along direction; negative erodes */ \
    float sigma;          /* blur pass: gaussian standard deviation in texels */ \
    int   taps;           /* blur pass: gaussian half-width in taps */ \
    int   fromSilhouette; /* 1 = source is the shape texture (.a), 0 = a field texture (.r/.g) */ \
    int   invert;         /* 1 = complement the field (first pass of an inset chain) */

// The helpers below are fragment-stage only (derivatives, samplers); the
// vertex shader defines QUAD_VERTEX to leave them out.
#ifndef QUAD_VERTEX

// The derivatives are taken per pixel, as on Metal, so the antialiasing ramp
// is the same width on both backends.
float _Fwidth(float value) {
    return fwidthFine(value);
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
float _roundBoxDistance(vec2 localPos, vec2 size, vec4 corners) {
    vec2 edges = max(-localPos, (localPos - size));
    float distance = (length(max(edges, 0.0)) + min(max(edges.x, edges.y), 0.0));
    if ((localPos.x < corners.x) && (localPos.y < corners.x)) { // top-left
        distance = max(distance, (length(localPos - vec2(corners.x, corners.x)) - corners.x));
    }
    if ((localPos.x > (size.x - corners.y)) && (localPos.y < corners.y)) { // top-right
        distance = max(distance, (length(localPos - vec2((size.x - corners.y), corners.y)) - corners.y));
    }
    if ((localPos.x > (size.x - corners.z)) && (localPos.y > (size.y - corners.z))) { // bottom-right
        distance = max(distance, (length(localPos - vec2((size.x - corners.z), (size.y - corners.z))) - corners.z));
    }
    if ((localPos.x < corners.w) && (localPos.y > (size.y - corners.w))) { // bottom-left
        distance = max(distance, (length(localPos - vec2(corners.w, (size.y - corners.w))) - corners.w));
    }
    return distance;
}

// Half-pixel linear edge profile of a signed distance (negative inside),
// normalised by its screen-space gradient.
float _edgeCoverage(float d) {
    return clamp((0.5 - (d / max(_Fwidth(d), 0.0001))), 0.0, 1.0);
}

// A sharp box is covered per axis: the product of the two one-axis coverages.
float _quadCoverage(vec2 localPos, vec2 size, vec4 corners) {
    if (any(greaterThan(corners, vec4(0.0)))) {
        return _edgeCoverage(_roundBoxDistance(localPos, size, corners));
    }
    vec2 edges = max(-localPos, (localPos - size));
    return (_edgeCoverage(edges.x) * _edgeCoverage(edges.y));
}

// Anti-aliased coverage of the shape at a fragment, in shape-local space.
float _coverage(vec2 localPos, vec4 bounds, vec4 borders, vec4 corners, int type) {
    if (type == 1) { // ellipse
        vec2 c = (bounds.zw * 0.5);
        return _edgeCoverage(length((localPos - c) / c) - 1.0);
    } else if (type == 2) { // quad outline (per-edge)
        vec4  b        = borders; // left, top, right, bottom
        float outerCov = _quadCoverage(localPos, bounds.zw, corners);
        if (any(greaterThan(corners, vec4(0.0)))) { // rounded: outer box minus inset inner box
            float innerCov  = 0.0;
            vec2  innerSize = (bounds.zw - vec2((b.x + b.z), (b.y + b.w)));
            if (all(greaterThan(innerSize, vec2(0.0)))) {
                vec4 innerCorners = max(vec4(
                    (corners.x - max(b.x, b.y)),  // TL - max(left, top)
                    (corners.y - max(b.z, b.y)),  // TR - max(right, top)
                    (corners.z - max(b.z, b.w)),  // BR - max(right, bottom)
                    (corners.w - max(b.x, b.w))   // BL - max(left, bottom)
                ), vec4(0.0));
                innerCov = _edgeCoverage(_roundBoxDistance((localPos - b.xy), innerSize, innerCorners));
            }
            return (outerCov * (1.0 - innerCov));
        }
        float big = (max(bounds.z, bounds.w) + 1.0);
        float dL  = (b.x > 0.0) ? (localPos.x - b.x)              : big;
        float dT  = (b.y > 0.0) ? (localPos.y - b.y)              : big;
        float dR  = (b.z > 0.0) ? ((bounds.z - b.z) - localPos.x) : big;
        float dB  = (b.w > 0.0) ? ((bounds.w - b.w) - localPos.y) : big;
        float holeCov = (_edgeCoverage(-min(dL, dR)) * _edgeCoverage(-min(dT, dB))); // 1 inside the hole
        return (outerCov * (1.0 - holeCov));
    } else if (type == 3) { // ellipse outline
        vec2  c  = (bounds.zw * 0.5);
        vec2  ci = max((c - borders.x), vec2(0.0001));
        float outerCov = _edgeCoverage(length((localPos - c) / c)  - 1.0);
        float innerCov = _edgeCoverage(length((localPos - c) / ci) - 1.0);
        return (outerCov * (1.0 - innerCov));
    }
    return _quadCoverage(localPos, bounds.zw, corners); // filled quad
}

float _gaussianWeight(int i, float sigma) {
    float x = float(i);
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

// Shadow field (x) and carried silhouette mask (y) at uv; see Painter.metal.
vec2 _shadowField(sampler2D source, vec2 uv, int fromSilhouette, int invert) {
    vec4 t = texture(source, uv);
    vec2 f = (fromSilhouette != 0) ? t.aa : t.rg;
    f.x = (invert != 0) ? (1.0 - f.x) : f.x;
    return f;
}

#endif

// Interpolated outputs of QuadVertex.vert.
#define QUAD_VARYINGS(direction)                                \
    layout(location = 0) direction vec2 vLocalPos;              \
    layout(location = 1) direction vec2 vLocalUV;               \
    layout(location = 2) direction vec2 vScreenUV;
