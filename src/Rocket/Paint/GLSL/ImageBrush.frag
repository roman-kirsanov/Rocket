/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#version 450

#include "Painter.glsl"

layout(std140, set = 3, binding = 0) uniform StateUniforms { STATE_UNIFORMS } uState;
layout(std140, set = 3, binding = 1) uniform ImageBrushUniforms {
    vec4 color;
    vec2 size;
    vec2 flip;
    vec2 repeat;
    vec4 npatch;
    vec4 source;
    vec4 destin;
} uBrush;
layout(std140, set = 3, binding = 2) uniform ShapeUniforms { SHAPE_UNIFORMS } uShape;
layout(set = 2, binding = 0) uniform sampler2D texture0;

QUAD_VARYINGS(in)

layout(location = 0) out vec4 outColor;

// Same as imageBrushFragment in Painter.metal, except that the texture is
// sampled before the fragments that contribute nothing are dropped: Vulkan
// leaves the implicit derivatives undefined after a divergent return, so every
// step up to the sample runs for every fragment and the drops come last.
void main() {
    float coverage = _coverage(vLocalPos, uShape.bounds, uShape.borders, uShape.corners, uShape.type);

    // The antialiasing fringe lies just outside the shape: clamp it back in so
    // it samples the edge texel instead of failing the dest test below.
    vec2 srcSize = (uBrush.source.zw - uBrush.source.xy);
    vec2 dstSize = (uBrush.destin.zw - uBrush.destin.xy);
    vec2 dstPos  = (clamp(vLocalPos, vec2(0.0), (uShape.bounds.zw * 0.999999)) - uBrush.destin.xy);

    // nPatch insets are shared by the source slicing and the dest frame; see
    // Painter.metal for the CSS border-image style overflow guard.
    vec2 srcFit = min(vec2(1.0), (srcSize / max((uBrush.npatch.xy + uBrush.npatch.zw), vec2(0.0001))));
    vec2 dstFit = min(vec2(1.0), (dstSize / max((uBrush.npatch.xy + uBrush.npatch.zw), vec2(0.0001))));
    vec4 srcNP  = (uBrush.npatch * srcFit.xyxy);
    vec4 dstNP  = (uBrush.npatch * dstFit.xyxy);

    // Floor-modulo so tiles before the image origin continue the pattern.
    if (uBrush.repeat.x == 1.0) dstPos.x -= (dstSize.x * floor(dstPos.x / dstSize.x));
    if (uBrush.repeat.y == 1.0) dstPos.y -= (dstSize.y * floor(dstPos.y / dstSize.y));

    bool outside = ((uBrush.repeat.x != 1.0) && ((dstPos.x < 0.0) || (dstPos.x >= dstSize.x)))
                || ((uBrush.repeat.y != 1.0) && ((dstPos.y < 0.0) || (dstPos.y >= dstSize.y)));

    vec2 srcPos = vec2(
        _npatchAxis(dstPos.x, srcSize.x, dstSize.x, srcNP.x, srcNP.z, dstNP.x, dstNP.z),
        _npatchAxis(dstPos.y, srcSize.y, dstSize.y, srcNP.y, srcNP.w, dstNP.y, dstNP.w)
    );
    vec2 suv = ((uBrush.source.xy + srcPos) / uBrush.size.xy);

    if (uBrush.flip.x == 1.0) suv.x = ((uBrush.source.z - srcPos.x) / uBrush.size.x);
    if (uBrush.flip.y == 1.0) suv.y = ((uBrush.source.w - srcPos.y) / uBrush.size.y);

    vec4 texel = texture(texture0, suv);

    if ((coverage <= 0.0) || outside) {
        outColor = vec4(0.0);
        return;
    }

    if ((texel.a > 0.0) && (uBrush.color.a > 0.0)) {
        float a = (texel.a * uBrush.color.a);
        texel = vec4((uBrush.color.rgb * a), a);
    }

    outColor = (texel * uState.opacity * coverage);
}
