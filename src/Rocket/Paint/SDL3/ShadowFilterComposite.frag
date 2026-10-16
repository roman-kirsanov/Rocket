/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#version 450

#include "Painter.glsl"

layout(std140, set = 3, binding = 1) uniform ShadowFilterUniforms {
    vec4  color;
    float opacity;
    vec2  texel;
    vec2  offset;         // inset field displacement, in pixels
    float sigma;          // gaussian standard deviation in texels
    int   taps;           // gaussian half-width in taps
    int   inset;          // 0 = drop shadow, 1 = inset
    int   fromSilhouette; // 1 = source is the shape texture (.a), 0 = a field texture (.r/.g)
    int   invert;         // 1 = complement the field (no earlier pass did it)
} uFilter;
layout(set = 2, binding = 0) uniform sampler2D texture0;

QUAD_VARYINGS(in)

layout(location = 0) out vec4 outColor;

// Vertical gaussian blur of the field, tinted and composited onto the target.
void main() {
    float sigma = max(uFilter.sigma, 0.0001);
    float sum   = 0.0;
    float wsum  = 0.0;

    // A drop shadow is displaced by the vertex transform; an inset shadow
    // shifts the sampled field here instead.
    vec2 fieldUV = vScreenUV;
    if (uFilter.inset != 0) {
        fieldUV -= (uFilter.offset * uFilter.texel);
    }

    for (int i = -uFilter.taps; i <= uFilter.taps; i++) {
        float w  = _gaussianWeight(i, sigma);
        vec2  uv = (fieldUV + vec2(0.0, float(i) * uFilter.texel.y));
        sum  += (_shadowField(texture0, uv, uFilter.fromSilhouette, uFilter.invert).x * w);
        wsum += w;
    }

    float aFinal = (wsum > 0.0) ? (sum / wsum) : 0.0;

    if (uFilter.inset != 0) {
        aFinal *= _shadowField(texture0, vScreenUV, uFilter.fromSilhouette, 0).y; // clip the inset shadow to the silhouette
    }

    float outA = aFinal * uFilter.color.a * uFilter.opacity;

    outColor = vec4((uFilter.color.rgb * outA), outA);
}
