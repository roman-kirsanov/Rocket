/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#version 450

#include "Painter.glsl"

layout(std140, set = 3, binding = 0) uniform StateUniforms { STATE_UNIFORMS } uState;
layout(std140, set = 3, binding = 1) uniform ColorBrushUniforms { vec4 color; } uBrush;
layout(std140, set = 3, binding = 2) uniform ShapeUniforms { SHAPE_UNIFORMS } uShape;

QUAD_VARYINGS(in)

layout(location = 0) out vec4 outColor;

void main() {
    float a = (uBrush.color.a * uState.opacity * _coverage(vLocalPos, uShape.bounds, uShape.borders, uShape.corners, uShape.type));
    outColor = vec4((uBrush.color.rgb * a), a);
}
