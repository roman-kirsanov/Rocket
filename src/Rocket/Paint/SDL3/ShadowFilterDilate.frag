/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#version 450

#include "Painter.glsl"

layout(std140, set = 3, binding = 1) uniform ShadowFilterFieldUniforms { SHADOW_FILTER_FIELD_UNIFORMS } uFilter;
layout(set = 2, binding = 0) uniform sampler2D texture0;

QUAD_VARYINGS(in)

layout(location = 0) out vec4 outColor;

// Separable dilation (positive spread) or erosion (negative spread) of the
// field along one axis; see Painter.metal.
void main() {
    vec2  step   = (uFilter.direction * uFilter.texel);
    float extent = abs(uFilter.spread);
    int   n      = int(floor(extent));
    vec2  center = _shadowField(texture0, vScreenUV, uFilter.fromSilhouette, uFilter.invert);
    float value  = center.x;

    for (int i = 1; i <= n; i++) {
        float a = _shadowField(texture0, (vScreenUV + (step * float(i))), uFilter.fromSilhouette, uFilter.invert).x;
        float b = _shadowField(texture0, (vScreenUV - (step * float(i))), uFilter.fromSilhouette, uFilter.invert).x;
        value = (uFilter.spread > 0.0) ? max(value, max(a, b)) : min(value, min(a, b));
    }

    if (extent > float(n)) {
        float a = _shadowField(texture0, (vScreenUV + (step * extent)), uFilter.fromSilhouette, uFilter.invert).x;
        float b = _shadowField(texture0, (vScreenUV - (step * extent)), uFilter.fromSilhouette, uFilter.invert).x;
        value = (uFilter.spread > 0.0) ? max(value, max(a, b)) : min(value, min(a, b));
    }

    outColor = vec4(value, center.y, 0.0, 1.0);
}
