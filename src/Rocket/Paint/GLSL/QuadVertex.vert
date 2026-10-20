/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#version 450

#define QUAD_VERTEX
#include "Painter.glsl"

layout(std140, set = 1, binding = 0) uniform StateUniforms { STATE_UNIFORMS } uState;
layout(std140, set = 1, binding = 1) uniform ShapeUniforms { SHAPE_UNIFORMS } uShape;

QUAD_VARYINGS(out)

void main() {
    vec2 vertex = uShape.vertices[gl_VertexIndex].xy;

    gl_Position = vec4((uState.projection * uState.transform * vec3(vertex, 1.0)), 1.0);
    vLocalPos   = (vertex - uShape.bounds.xy);
    vScreenUV   = (vertex / uState.resolution);
    vLocalUV    = (vLocalPos / uShape.bounds.zw);
}
