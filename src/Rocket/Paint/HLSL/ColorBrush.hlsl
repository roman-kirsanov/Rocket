/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "Painter.hlsli"

struct ColorBrushUniforms {
    float4 color;
};

ConstantBuffer<StateUniforms>      uState : register(b0, space3);
ConstantBuffer<ColorBrushUniforms> uBrush : register(b1, space3);
ConstantBuffer<ShapeUniforms>      uShape : register(b2, space3);

float4 main(QuadVaryings input) : SV_Target {
    float a = (uBrush.color.a * uState.opacity * _coverage(input.localPos, uShape.bounds, uShape.borders, uShape.corners, uShape.type));
    return float4((uBrush.color.rgb * a), a);
}
