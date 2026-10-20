/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "Painter.hlsli"

struct ImageBrushUniforms {
    float4 color;
    float2 size;
    float2 flip;
    float2 repeat;
    float4 npatch;
    float4 source;
    float4 destin;
};

ConstantBuffer<StateUniforms>      uState : register(b0, space3);
ConstantBuffer<ImageBrushUniforms> uBrush : register(b1, space3);
ConstantBuffer<ShapeUniforms>      uShape : register(b2, space3);
Texture2D<float4>                  texture0 : register(t0, space2);
SamplerState                       sampler0 : register(s0, space2);

// Same as imageBrushFragment in Painter.metal, except that the texture is
// sampled before the fragments that contribute nothing are dropped: the
// implicit derivatives are undefined after a divergent return, so every step
// up to the sample runs for every fragment and the drops come last.
float4 main(QuadVaryings input) : SV_Target {
    float coverage = _coverage(input.localPos, uShape.bounds, uShape.borders, uShape.corners, uShape.type);

    // The antialiasing fringe lies just outside the shape: clamp it back in so
    // it samples the edge texel instead of failing the dest test below.
    float2 srcSize = (uBrush.source.zw - uBrush.source.xy);
    float2 dstSize = (uBrush.destin.zw - uBrush.destin.xy);
    float2 dstPos  = (clamp(input.localPos, 0.0, (uShape.bounds.zw * 0.999999)) - uBrush.destin.xy);

    // nPatch insets are shared by the source slicing and the dest frame; see
    // Painter.metal for the CSS border-image style overflow guard.
    float2 srcFit = min(1.0, (srcSize / max((uBrush.npatch.xy + uBrush.npatch.zw), 0.0001)));
    float2 dstFit = min(1.0, (dstSize / max((uBrush.npatch.xy + uBrush.npatch.zw), 0.0001)));
    float4 srcNP  = (uBrush.npatch * srcFit.xyxy);
    float4 dstNP  = (uBrush.npatch * dstFit.xyxy);

    // Floor-modulo so tiles before the image origin continue the pattern.
    if (uBrush.repeat.x == 1.0) dstPos.x -= (dstSize.x * floor(dstPos.x / dstSize.x));
    if (uBrush.repeat.y == 1.0) dstPos.y -= (dstSize.y * floor(dstPos.y / dstSize.y));

    bool outside = ((uBrush.repeat.x != 1.0) && ((dstPos.x < 0.0) || (dstPos.x >= dstSize.x)))
                || ((uBrush.repeat.y != 1.0) && ((dstPos.y < 0.0) || (dstPos.y >= dstSize.y)));

    float2 srcPos = float2(
        _npatchAxis(dstPos.x, srcSize.x, dstSize.x, srcNP.x, srcNP.z, dstNP.x, dstNP.z),
        _npatchAxis(dstPos.y, srcSize.y, dstSize.y, srcNP.y, srcNP.w, dstNP.y, dstNP.w)
    );
    float2 suv = ((uBrush.source.xy + srcPos) / uBrush.size.xy);

    if (uBrush.flip.x == 1.0) suv.x = ((uBrush.source.z - srcPos.x) / uBrush.size.x);
    if (uBrush.flip.y == 1.0) suv.y = ((uBrush.source.w - srcPos.y) / uBrush.size.y);

    float4 texel = texture0.Sample(sampler0, suv);

    if ((coverage <= 0.0) || outside) {
        return float4(0.0, 0.0, 0.0, 0.0);
    }

    if ((texel.a > 0.0) && (uBrush.color.a > 0.0)) {
        float a = (texel.a * uBrush.color.a);
        texel = float4((uBrush.color.rgb * a), a);
    }

    return (texel * uState.opacity * coverage);
}
