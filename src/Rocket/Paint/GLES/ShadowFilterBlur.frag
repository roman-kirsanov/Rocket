/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

layout(std140) uniform ShadowFilterFieldUniforms { SHADOW_FILTER_FIELD_UNIFORMS } uFilter;
uniform sampler2D texture0;

QUAD_VARYINGS(in)

layout(location = 0) out vec4 outColor;

void main() {
    float sigma = max(uFilter.sigma, 0.0001);
    vec2  stride  = (uFilter.direction * uFilter.texel);
    float sum   = 0.0;
    float wsum  = 0.0;

    for (int i = -uFilter.taps; i <= uFilter.taps; i++) {
        float w = _gaussianWeight(i, sigma);
        sum  += (_shadowField(texture0, (vScreenUV + (stride * float(i))), uFilter.fromSilhouette, uFilter.invert).x * w);
        wsum += w;
    }

    float mask = _shadowField(texture0, vScreenUV, uFilter.fromSilhouette, 0).y;

    outColor = vec4(((wsum > 0.0) ? (sum / wsum) : 0.0), mask, 0.0, 1.0);
}
