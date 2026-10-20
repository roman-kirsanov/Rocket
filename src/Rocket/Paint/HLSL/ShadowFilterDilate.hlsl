/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "Painter.hlsli"

ConstantBuffer<ShadowFilterFieldUniforms> uFilter : register(b1, space3);
Texture2D<float4>                         texture0 : register(t0, space2);
SamplerState                              sampler0 : register(s0, space2);

float4 main(QuadVaryings input) : SV_Target {
    float2 step   = (uFilter.direction * uFilter.texel);
    float  extent = abs(uFilter.spread);
    int    n      = (int)floor(extent);
    float2 center = _shadowField(texture0, sampler0, input.screenUV, uFilter.fromSilhouette, uFilter.invert);
    float  value  = center.x;

    for (int i = 1; i <= n; i++) {
        float a = _shadowField(texture0, sampler0, (input.screenUV + (step * (float)i)), uFilter.fromSilhouette, uFilter.invert).x;
        float b = _shadowField(texture0, sampler0, (input.screenUV - (step * (float)i)), uFilter.fromSilhouette, uFilter.invert).x;
        value = (uFilter.spread > 0.0) ? max(value, max(a, b)) : min(value, min(a, b));
    }

    if (extent > (float)n) {
        float a = _shadowField(texture0, sampler0, (input.screenUV + (step * extent)), uFilter.fromSilhouette, uFilter.invert).x;
        float b = _shadowField(texture0, sampler0, (input.screenUV - (step * extent)), uFilter.fromSilhouette, uFilter.invert).x;
        value = (uFilter.spread > 0.0) ? max(value, max(a, b)) : min(value, min(a, b));
    }

    return float4(value, center.y, 0.0, 1.0);
}
