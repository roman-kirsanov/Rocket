/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "Painter.hlsli"

struct BlurFilterUniforms {
    float2 texelSize;
    float2 direction;
    float  sigma;
    int    taps;
};

ConstantBuffer<BlurFilterUniforms> uFilter : register(b1, space3);
Texture2D<float4>                  texture0 : register(t0, space2);
SamplerState                       sampler0 : register(s0, space2);

float4 main(QuadVaryings input) : SV_Target {
    float4 sum       = float4(0.0, 0.0, 0.0, 0.0);
    float  weightSum = 0.0;

    for (int i = -uFilter.taps; i <= uFilter.taps; i++) {
        float weight = exp(-((float)i * (float)i) / (2.0 * uFilter.sigma * uFilter.sigma));
        sum += (texture0.Sample(sampler0, (input.screenUV + (uFilter.direction * uFilter.texelSize * (float)i))) * weight);
        weightSum += weight;
    }

    return (sum / weightSum);
}
