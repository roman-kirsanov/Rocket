#include <metal_stdlib>
#include "Painter.metal.hpp"

using namespace metal;

struct VertexResult {
    float4 position [[position]];
    float2 localPos;
    float2 localUV;
    float2 screenUV;
};

// Signed distance (pixels, negative inside) from a rounded box whose top-left
// corner sits at the local origin (Y down). corners: x=TL, y=TR, z=BR, w=BL.
//
// Radius selection is by corner REGION (the axis-aligned radius-sized square
// at each corner), not by quadrant: the CPU-side CSS overlap rule only caps
// ADJACENT radius sums by the side lengths, so a lone radius may exceed half
// a box dimension and its arc then crosses the box midlines, where quadrant
// selection would switch to the wrong radius. Adjacent corner squares are
// disjoint under that rule; DIAGONAL sums are not constrained, so diagonal
// squares may still overlap — hence each in-region corner constraint is
// max-combined with the plain box SDF instead of if/else-selected: overlapping
// corner cuts compose as an intersection, the field stays continuous across
// region seams (the box term dominates there), and zero-radius corners fall
// through to the sharp box distance.
static inline float _roundBoxDistance(float2 localPos, float2 size, float4 corners) {
    float2 edges = max(-localPos, (localPos - size));
    float distance = (length(max(edges, 0.0f)) + min(max(edges.x, edges.y), 0.0f));
    if ((localPos.x < corners.x) && (localPos.y < corners.x)) { // top-left
        distance = max(distance, (length(localPos - float2(corners.x, corners.x)) - corners.x));
    }
    if ((localPos.x > (size.x - corners.y)) && (localPos.y < corners.y)) { // top-right
        distance = max(distance, (length(localPos - float2((size.x - corners.y), corners.y)) - corners.y));
    }
    if ((localPos.x > (size.x - corners.z)) && (localPos.y > (size.y - corners.z))) { // bottom-right
        distance = max(distance, (length(localPos - float2((size.x - corners.z), (size.y - corners.z))) - corners.z));
    }
    if ((localPos.x < corners.w) && (localPos.y > (size.y - corners.w))) { // bottom-left
        distance = max(distance, (length(localPos - float2(corners.w, (size.y - corners.w))) - corners.w));
    }
    return distance;
}

// Anti-aliased coverage of the shape at a fragment, in shape-local space.
static inline float _coverage(float2 localPos, ShapeUniforms constant& uShape) {
    if (uShape.type == 1) { // ellipse
        float2 c  = (uShape.bounds.zw * 0.5f);
        float  d  = length((localPos - c) / c);
        float  aa = max(fwidth(d), 0.0001f);
        return (1.0f - smoothstep((1.0f - aa), (1.0f + aa), d));
    } else if (uShape.type == 2) { // quad outline (per-edge)
        if (any(uShape.corners > float4(0.0f))) { // rounded: outer box minus inset inner box
            // Inner per-corner radius rule: innerR = max(outerR - max(adjacent borders), 0),
            // e.g. inner TL = max(TL - max(left, top), 0). An inner QuadShape inset by the
            // border widths with that radius fits this outline seamlessly.
            float4 b     = uShape.borders; // left, top, right, bottom
            float  dOut  = _roundBoxDistance(localPos, uShape.bounds.zw, uShape.corners);
            float  aaOut = max(fwidth(dOut), 0.0001f);
            float  outerCov = (1.0f - smoothstep(-aaOut, aaOut, dOut));
            float  innerCov = 0.0f;
            float2 innerSize = (uShape.bounds.zw - float2((b.x + b.z), (b.y + b.w)));
            if (all(innerSize > float2(0.0f))) {
                float4 innerCorners = max(float4(
                    (uShape.corners.x - max(b.x, b.y)),  // TL - max(left, top)
                    (uShape.corners.y - max(b.z, b.y)),  // TR - max(right, top)
                    (uShape.corners.z - max(b.z, b.w)),  // BR - max(right, bottom)
                    (uShape.corners.w - max(b.x, b.w))   // BL - max(left, bottom)
                ), float4(0.0f));
                float dIn  = _roundBoxDistance((localPos - b.xy), innerSize, innerCorners);
                float aaIn = max(fwidth(dIn), 0.0001f);
                innerCov = (1.0f - smoothstep(-aaIn, aaIn, dIn));
            }
            return (outerCov * (1.0f - innerCov));
        }
        float big = (max(uShape.bounds.z, uShape.bounds.w) + 1.0f);
        float dL  = (uShape.borders.x > 0.0f) ? (localPos.x - uShape.borders.x)                    : big;
        float dT  = (uShape.borders.y > 0.0f) ? (localPos.y - uShape.borders.y)                    : big;
        float dR  = (uShape.borders.z > 0.0f) ? ((uShape.bounds.z - uShape.borders.z) - localPos.x) : big;
        float dB  = (uShape.borders.w > 0.0f) ? ((uShape.bounds.w - uShape.borders.w) - localPos.y) : big;
        float dIn = min(min(dL, dR), min(dT, dB));
        float aa  = max(fwidth(dIn), 0.0001f);
        return (1.0f - smoothstep(0.0f, aa, dIn));
    } else if (uShape.type == 3) { // ellipse outline
        float2 c   = (uShape.bounds.zw * 0.5f);
        float2 ci  = max((c - uShape.borders.x), float2(0.0001f));
        float  d   = length((localPos - c) / c);
        float  di  = length((localPos - c) / ci);
        float  aa  = max(fwidth(d),  0.0001f);
        float  aai = max(fwidth(di), 0.0001f);
        float outerCov = (1.0f - smoothstep((1.0f - aa),  (1.0f + aa),  d));
        float innerCov = (1.0f - smoothstep((1.0f - aai), (1.0f + aai), di));
        return (outerCov * (1.0f - innerCov));
    }
    if (any(uShape.corners > float4(0.0f))) { // filled quad, rounded corners
        float d  = _roundBoxDistance(localPos, uShape.bounds.zw, uShape.corners);
        float aa = max(fwidth(d), 0.0001f);
        return (1.0f - smoothstep(-aa, aa, d));
    }
    return 1.0f; // filled quad
}

static inline float _gaussianWeight(int i, float sigma) {
    float x = float(i);
    return exp(-(x * x) / (2.0f * sigma * sigma));
}

static inline float _applySpreadXY(texture2d<float> texture, sampler sampler, float2 uv, float2 texel, float2 spread) {
    float2 d = spread * texel;

    if (all(spread == float2(0.0f))) {
        return texture.sample(sampler, uv).r;
    }

    float a0 = texture.sample(sampler, uv).r;
    float a1 = texture.sample(sampler, uv + float2(+d.x, 0.0f)).r;
    float a2 = texture.sample(sampler, uv + float2(-d.x, 0.0f)).r;
    float a3 = texture.sample(sampler, uv + float2(0.0f, +d.y)).r;
    float a4 = texture.sample(sampler, uv + float2(0.0f, -d.y)).r;
    float a5 = texture.sample(sampler, uv + float2(+d.x, +d.y)).r;
    float a6 = texture.sample(sampler, uv + float2(+d.x, -d.y)).r;
    float a7 = texture.sample(sampler, uv + float2(-d.x, +d.y)).r;
    float a8 = texture.sample(sampler, uv + float2(-d.x, -d.y)).r;

    return max(a0, max(max(max(a1, a2), max(a3, a4)), max(max(a5, a6), max(a7, a8))));
}

vertex VertexResult quadVertex(uint vid [[vertex_id]], StateUniforms constant& uState [[buffer(0)]], ShapeUniforms constant& uShape [[buffer(1)]]) {
    VertexResult ret = {};

    ret.position = float4(uState.projection * uState.transform * float3(uShape.vertices[vid], 1.0), 1.0);
    ret.localPos = (uShape.vertices[vid] - uShape.bounds.xy);
    ret.screenUV = (uShape.vertices[vid] / uState.resolution);
    ret.localUV  = (ret.localPos / uShape.bounds.zw);

    return ret;
}

fragment float4 colorFragment(VertexResult in [[stage_in]], StateUniforms constant& uState [[buffer(0)]], ColorUniforms constant& uBrush [[buffer(1)]], ShapeUniforms constant& uShape [[buffer(2)]]) {
    float a = (uBrush.color.a * uState.opacity * _coverage(in.localPos, uShape));
    return float4((uBrush.color.rgb * a), a);
}

fragment float4 imageFragment(VertexResult in [[stage_in]], StateUniforms constant& uState [[buffer(0)]], ImageUniforms constant& uBrush [[buffer(1)]], ShapeUniforms constant& uShape [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler sampler0 [[sampler(0)]]) {
    float2 srcSize = (uBrush.source.zw - uBrush.source.xy);
    float2 dstSize = (uBrush.destin.zw - uBrush.destin.xy);
    float2 dstPos  = (in.localPos - uBrush.destin.xy);

    float4 src = float4(0.0);
    float4 dst = float4(0.0);

    // nPatch insets are shared by the source slicing and the dest frame: real
    // consumers swap in a matching 1x/2x/3x sprite and scale the insets together
    // with it, so both spaces agree. Guard against degenerate values CSS
    // border-image style: when opposite insets overflow the source (or dest)
    // extent on an axis, scale both proportionally so they exactly meet —
    // independently for the source and dest spaces.
    float2 srcFit = min(float2(1.0), (srcSize / max((uBrush.npatch.xy + uBrush.npatch.zw), float2(0.0001))));
    float2 dstFit = min(float2(1.0), (dstSize / max((uBrush.npatch.xy + uBrush.npatch.zw), float2(0.0001))));
    float4 srcNP  = (uBrush.npatch * srcFit.xyxy);
    float4 dstNP  = (uBrush.npatch * dstFit.xyxy);

    float4 srcMM = float4(srcNP.x, srcNP.y, (srcSize.x - srcNP.z), (srcSize.y - srcNP.w));
    float4 dstMM = float4(dstNP.x, dstNP.y, (dstSize.x - dstNP.z), (dstSize.y - dstNP.w));

    if (uBrush.repeat.x == 1.0) dstPos.x = abs(fmod(dstPos.x, dstSize.x));
    if (uBrush.repeat.y == 1.0) dstPos.y = abs(fmod(dstPos.y, dstSize.y));

    // Without repeat, fragments outside the positioned image rect contribute
    // nothing: extrapolated UVs would otherwise hit the ClampToEdge sampler and
    // smear the border texel across the rest of the quad. Premultiplied zero
    // composes as fully transparent under the "over" blend.
    if ((uBrush.repeat.x != 1.0) && ((dstPos.x < 0.0) || (dstPos.x >= dstSize.x))) {
        return float4(0.0);
    }

    if ((uBrush.repeat.y != 1.0) && ((dstPos.y < 0.0) || (dstPos.y >= dstSize.y))) {
        return float4(0.0);
    }

    if (all(dstPos >= dstMM.xy) && all(dstPos < dstMM.zw)) {
        src = srcMM;
        dst = dstMM;
    } else {
        float4 srcTL = float4(0.0, 0.0, srcNP.x, srcNP.y);
        float4 dstTL = float4(0.0, 0.0, dstNP.x, dstNP.y);
        if (all(dstPos >= dstTL.xy) && all(dstPos < dstTL.zw)) {
            src = srcTL; dst = dstTL;
        } else {
            float4 srcTM = float4(srcNP.x, 0.0, (srcSize.x - srcNP.z), srcNP.y);
            float4 dstTM = float4(dstNP.x, 0.0, (dstSize.x - dstNP.z), dstNP.y);
            if (all(dstPos >= dstTM.xy) && all(dstPos < dstTM.zw)) {
                src = srcTM; dst = dstTM;
            } else {
                float4 srcTR = float4((srcSize.x - srcNP.z), 0.0, srcSize.x, srcNP.y);
                float4 dstTR = float4((dstSize.x - dstNP.z), 0.0, dstSize.x, dstNP.y);
                if (all(dstPos >= dstTR.xy) && all(dstPos < dstTR.zw)) {
                    src = srcTR; dst = dstTR;
                } else {
                    float4 srcML = float4(0.0, srcNP.y, srcNP.x, (srcSize.y - srcNP.w));
                    float4 dstML = float4(0.0, dstNP.y, dstNP.x, (dstSize.y - dstNP.w));
                    if (all(dstPos >= dstML.xy) && all(dstPos < dstML.zw)) {
                        src = srcML; dst = dstML;
                    } else {
                        float4 srcMR = float4((srcSize.x - srcNP.z), srcNP.y, srcSize.x, (srcSize.y - srcNP.w));
                        float4 dstMR = float4((dstSize.x - dstNP.z), dstNP.y, dstSize.x, (dstSize.y - dstNP.w));
                        if (all(dstPos >= dstMR.xy) && all(dstPos < dstMR.zw)) {
                            src = srcMR; dst = dstMR;
                        } else {
                            float4 srcBL = float4(0.0, (srcSize.y - srcNP.w), srcNP.x, srcSize.y);
                            float4 dstBL = float4(0.0, (dstSize.y - dstNP.w), dstNP.x, dstSize.y);
                            if (all(dstPos >= dstBL.xy) && all(dstPos < dstBL.zw)) {
                                src = srcBL; dst = dstBL;
                            } else {
                                float4 srcBM = float4(srcNP.x, (srcSize.y - srcNP.w), (srcSize.x - srcNP.z), srcSize.y);
                                float4 dstBM = float4(dstNP.x, (dstSize.y - dstNP.w), (dstSize.x - dstNP.z), dstSize.y);
                                if (all(dstPos >= dstBM.xy) && all(dstPos < dstBM.zw)) {
                                    src = srcBM; dst = dstBM;
                                } else {
                                    float4 srcBR = float4((srcSize.x - srcNP.z), (srcSize.y - srcNP.w), srcSize.x, srcSize.y);
                                    float4 dstBR = float4((dstSize.x - dstNP.z), (dstSize.y - dstNP.w), dstSize.x, dstSize.y);
                                    if (all(dstPos >= dstBR.xy) && all(dstPos < dstBR.zw)) {
                                        src = srcBR; dst = dstBR;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    float2 duv = ((dstPos - dst.xy) / (dst.zw - dst.xy));
    float2 suv = ((uBrush.source.xy + mix(src.xy, src.zw, duv)) / uBrush.size.xy);

    if (uBrush.flip.x == 1.0) suv.x = ((uBrush.source.z - mix(src.x, src.z, duv.x)) / uBrush.size.x);
    if (uBrush.flip.y == 1.0) suv.y = ((uBrush.source.w - mix(src.y, src.w, duv.y)) / uBrush.size.y);

    float4 texel = texture0.sample(sampler0, suv);

    if ((texel.a > 0.0) && (uBrush.color.a > 0.0)) {
        float a = (texel.a * uBrush.color.a);
        texel = float4((uBrush.color.rgb * a), a);
    }

    return (texel * uState.opacity * _coverage(in.localPos, uShape));
}

fragment float4 linearGradientFragment(VertexResult in [[stage_in]], StateUniforms constant& uState [[buffer(0)]], GradientUniforms constant& uBrush [[buffer(1)]], ShapeUniforms constant& uShape [[buffer(2)]]) {
    float4 result = {};
    float2 dPoint = (uBrush.stopPoint - uBrush.startPoint);
    float t = (
        ((in.localUV.x - uBrush.startPoint.x) * dPoint.x) +
        ((in.localUV.y - uBrush.startPoint.y) * dPoint.y)
    ) / (
        (dPoint.x * dPoint.x) +
        (dPoint.y * dPoint.y)
    );

    float2 xPoint       = (uBrush.startPoint + (t * dPoint));
    float lineDistance  = distance(uBrush.startPoint, uBrush.stopPoint);
    float svexDistance  = distance(uBrush.startPoint, xPoint);
    float evexDistance  = distance(uBrush.stopPoint,  xPoint);

    if ((svexDistance < lineDistance) && (evexDistance < lineDistance)) {
        float tt = (svexDistance / lineDistance);
        for (int i = 0; i < (uBrush.stopCount + 1); i++) {
            float tLeft  = (i > 0              ? uBrush.stopPosition[i - 1] : 0.0f);
            float tRight = (i < uBrush.stopCount ? uBrush.stopPosition[i]   : 1.0f);
            if (tt >= tLeft && tt < tRight) {
                float4 leftColor  = (i > 0              ? uBrush.stopColors[i - 1] : uBrush.startColor);
                float4 rightColor = (i < uBrush.stopCount ? uBrush.stopColors[i]   : uBrush.stopColor);
                float ttt = ((tt - tLeft) / (tRight - tLeft));
                result = mix(leftColor, rightColor, ttt);
                break;
            }
        }
    } else if (svexDistance < evexDistance) {
        result = uBrush.startColor;
    } else if (svexDistance > evexDistance) {
        result = uBrush.stopColor;
    }

    float a = (result.a * uState.opacity * _coverage(in.localPos, uShape));
    return float4((result.rgb * a), a);
}

fragment float4 radialGradientFragment(VertexResult in [[stage_in]], StateUniforms constant& uState [[buffer(0)]], GradientUniforms constant& uBrush [[buffer(1)]], ShapeUniforms constant& uShape [[buffer(2)]]) {
    float4 result      = {};
    float lineDistance = distance(uBrush.startPoint, uBrush.stopPoint);
    float svexDistance = distance(uBrush.startPoint, in.localUV);

    if (svexDistance < lineDistance) {
        float t = (svexDistance / lineDistance);
        for (int i = 0; i < (uBrush.stopCount + 1); i++) {
            float tLeft  = (i > 0              ? uBrush.stopPosition[i - 1] : 0.0f);
            float tRight = (i < uBrush.stopCount ? uBrush.stopPosition[i]   : 1.0f);
            if (t >= tLeft && t < tRight) {
                float4 leftColor  = (i > 0              ? uBrush.stopColors[i - 1] : uBrush.startColor);
                float4 rightColor = (i < uBrush.stopCount ? uBrush.stopColors[i]   : uBrush.stopColor);
                float tt = ((t - tLeft) / (tRight - tLeft));
                result = mix(leftColor, rightColor, tt);
                break;
            }
        }
    } else {
        result = uBrush.stopColor;
    }

    float a = (result.a * uState.opacity * _coverage(in.localPos, uShape));
    return float4((result.rgb * a), a);
}

fragment float4 blurFragment(VertexResult in [[stage_in]], StateUniforms constant& uState [[buffer(0)]], FilterBlurUniforms constant& uBlur [[buffer(1)]], texture2d<float> texture0 [[texture(0)]], sampler sampler0 [[sampler(0)]]) {
    float4 sum      = float4(0.0);
    float weightSum = 0.0f;

    for (int i = -uBlur.taps; i <= uBlur.taps; i++) {
        float weight = exp(-(float(i) * float(i)) / (2.0f * uBlur.sigma * uBlur.sigma));
        sum += (texture0.sample(sampler0, (in.screenUV + (uBlur.direction * uBlur.texelSize * float(i)))) * weight);
        weightSum += weight;
    }

    return (sum / weightSum);
}

fragment float4 shadowPass1Fragment(VertexResult in [[stage_in]], BlurUniforms constant& uBlur [[buffer(1)]], texture2d<float> texture0 [[texture(0)]], sampler sampler0 [[sampler(0)]]) {
    float sigma = max(float(uBlur.radius) / 3.0f, 0.0001f);
    float sum   = 0.0f;
    float wsum  = 0.0f;

    for (int i = -int(uBlur.radius); i <= int(uBlur.radius); i++) {
        float w  = _gaussianWeight(i, sigma);
        float2 uv = (in.screenUV + float2(float(i) * uBlur.texel.x, 0.0f));
        float a  = texture0.sample(sampler0, uv).a;
        if (uBlur.inset != 0) {
            a = (1.0f - a); // inset casts the shadow from the shape's complement
        }
        sum  += (a * w);
        wsum += w;
    }

    return float4(((wsum > 0.0f) ? (sum / wsum) : 0.0f), 0.0f, 0.0f, 1.0f);
}

fragment float4 shadowPass2Fragment(VertexResult in [[stage_in]], ShadowUniforms constant& uShadow [[buffer(1)]], texture2d<float> texture0 [[texture(0)]], texture2d<float> texture1 [[texture(1)]], sampler sampler0 [[sampler(0)]]) {
    float sigma = max(float(uShadow.radius) / 3.0f, 0.0001f);
    float sum   = 0.0f;
    float wsum  = 0.0f;

    // A drop shadow displaces the whole quad by the offset (via the vertex transform),
    // so the field is sampled straight. An inset shadow keeps the quad in place and
    // shifts the sampled field here instead, so the silhouette mask stays aligned.
    float2 fieldUV = in.screenUV;
    if (uShadow.inset != 0) {
        fieldUV -= (uShadow.offset * uShadow.texel);
    }

    // Spread means dilate-then-blur: each y-blur tap samples the spread-dilated
    // pass-1 field (already x-blurred), so the dilated silhouette receives the
    // full y-blur too. With zero spread _applySpreadXY is a plain sample and the
    // result is the pure separable gaussian — identical ramps on both axes.
    for (int i = -int(uShadow.radius); i <= int(uShadow.radius); i++) {
        float w   = _gaussianWeight(i, sigma);
        float2 uv = (fieldUV + float2(0.0f, float(i) * uShadow.texel.y));
        float a   = _applySpreadXY(texture0, sampler0, uv, uShadow.texel, uShadow.spread);
        sum  += (a * w);
        wsum += w;
    }

    float aFinal = (wsum > 0.0f) ? (sum / wsum) : 0.0f;

    if (uShadow.inset != 0) {
        aFinal *= texture1.sample(sampler0, in.screenUV).a; // clip the inset shadow to the silhouette
    }

    float outA = aFinal * uShadow.color.a * uShadow.opacity;

    return float4((uShadow.color.rgb * outA), outA);
}
