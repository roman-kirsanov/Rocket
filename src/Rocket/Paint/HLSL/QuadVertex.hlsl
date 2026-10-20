/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define QUAD_VERTEX
#include "Painter.hlsli"

ConstantBuffer<StateUniforms> uState : register(b0, space1);
ConstantBuffer<ShapeUniforms> uShape : register(b1, space1);

QuadVaryings main(uint vertexIndex : SV_VertexID) {
    float2 vertex = uShape.vertices[vertexIndex].xy;

    QuadVaryings output;
    output.position = float4(mul(uState.projection, mul(uState.transform, float3(vertex, 1.0))), 1.0);
    output.localPos = (vertex - uShape.bounds.xy);
    output.screenUV = (vertex / uState.resolution);
    output.localUV  = (output.localPos / uShape.bounds.zw);
    return output;
}
