/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#version 450

#include "Painter.glsl"

layout(std140, set = 3, binding = 1) uniform BlurFilterUniforms {
    vec2  texelSize;
    vec2  direction;
    float sigma;
    int   taps;
} uFilter;
layout(set = 2, binding = 0) uniform sampler2D texture0;

QUAD_VARYINGS(in)

layout(location = 0) out vec4 outColor;

void main() {
    vec4  sum       = vec4(0.0);
    float weightSum = 0.0;

    for (int i = -uFilter.taps; i <= uFilter.taps; i++) {
        float weight = exp(-(float(i) * float(i)) / (2.0 * uFilter.sigma * uFilter.sigma));
        sum += (texture(texture0, (vScreenUV + (uFilter.direction * uFilter.texelSize * float(i)))) * weight);
        weightSum += weight;
    }

    outColor = (sum / weightSum);
}
