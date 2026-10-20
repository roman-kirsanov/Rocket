/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "Painter.hlsli"

struct ShadowFilterUniforms {
    float4 color;
    float  opacity;
    float  _pad0;          // std140 aligns the float2 below to 8 bytes; HLSL would pack it right after opacity
    float2 texel;
    float2 offset;         // inset field displacement, in pixels
    float  sigma;          // gaussian standard deviation in texels
    int    taps;           // gaussian half-width in taps
    int    inset;          // 0 = drop shadow, 1 = inset
    int    fromSilhouette; // 1 = source is the shape texture (.a), 0 = a field texture (.r/.g)
    int    invert;         // 1 = complement the field (no earlier pass did it)
};

ConstantBuffer<ShadowFilterUniforms> uFilter : register(b1, space3);
Texture2D<float4>                    texture0 : register(t0, space2);
SamplerState                         sampler0 : register(s0, space2);

float4 main(QuadVaryings input) : SV_Target {
    float sigma = max(uFilter.sigma, 0.0001);
    float sum   = 0.0;
    float wsum  = 0.0;

    // A drop shadow is displaced by the vertex transform; an inset shadow
    // shifts the sampled field here instead.
    float2 fieldUV = input.screenUV;
    if (uFilter.inset != 0) {
        fieldUV -= (uFilter.offset * uFilter.texel);
    }

    for (int i = -uFilter.taps; i <= uFilter.taps; i++) {
        float  w  = _gaussianWeight(i, sigma);
        float2 uv = (fieldUV + float2(0.0, (float)i * uFilter.texel.y));
        sum  += (_shadowField(texture0, sampler0, uv, uFilter.fromSilhouette, uFilter.invert).x * w);
        wsum += w;
    }

    float aFinal = (wsum > 0.0) ? (sum / wsum) : 0.0;

    if (uFilter.inset != 0) {
        aFinal *= _shadowField(texture0, sampler0, input.screenUV, uFilter.fromSilhouette, 0).y; // clip the inset shadow to the silhouette
    }

    float outA = aFinal * uFilter.color.a * uFilter.opacity;

    return float4((uFilter.color.rgb * outA), outA);
}
