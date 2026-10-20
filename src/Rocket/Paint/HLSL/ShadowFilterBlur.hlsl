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
    float  sigma = max(uFilter.sigma, 0.0001);
    float2 step  = (uFilter.direction * uFilter.texel);
    float  sum   = 0.0;
    float  wsum  = 0.0;

    for (int i = -uFilter.taps; i <= uFilter.taps; i++) {
        float w = _gaussianWeight(i, sigma);
        sum  += (_shadowField(texture0, sampler0, (input.screenUV + (step * (float)i)), uFilter.fromSilhouette, uFilter.invert).x * w);
        wsum += w;
    }

    float mask = _shadowField(texture0, sampler0, input.screenUV, uFilter.fromSilhouette, 0).y;

    return float4(((wsum > 0.0) ? (sum / wsum) : 0.0), mask, 0.0, 1.0);
}
