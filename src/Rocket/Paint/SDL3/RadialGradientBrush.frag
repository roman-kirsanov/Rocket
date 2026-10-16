/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#version 450

#include "Painter.glsl"

layout(std140, set = 3, binding = 0) uniform StateUniforms { STATE_UNIFORMS } uState;
layout(std140, set = 3, binding = 1) uniform GradientBrushUniforms { GRADIENT_BRUSH_UNIFORMS } uBrush;
layout(std140, set = 3, binding = 2) uniform ShapeUniforms { SHAPE_UNIFORMS } uShape;

QUAD_VARYINGS(in)

layout(location = 0) out vec4 outColor;

float _stopPosition(int i) {
    return uBrush.stopPositions[i / 4][i % 4];
}

void main() {
    vec4  result       = vec4(0.0);
    float lineDistance = distance(uBrush.startPoint, uBrush.stopPoint);
    float svexDistance = distance(uBrush.startPoint, vLocalUV);

    if (svexDistance < lineDistance) {
        float t = (svexDistance / lineDistance);
        for (int i = 0; i < (uBrush.stopCount + 1); i++) {
            float tLeft  = (i > 0                ? _stopPosition(i - 1) : 0.0);
            float tRight = (i < uBrush.stopCount ? _stopPosition(i)     : 1.0);
            if (t >= tLeft && t < tRight) {
                vec4 leftColor  = (i > 0                ? uBrush.stopColors[i - 1] : uBrush.startColor);
                vec4 rightColor = (i < uBrush.stopCount ? uBrush.stopColors[i]     : uBrush.stopColor);
                float tt = ((t - tLeft) / (tRight - tLeft));
                result = mix(leftColor, rightColor, tt);
                break;
            }
        }
    } else {
        result = uBrush.stopColor;
    }

    outColor = (result * uState.opacity * _coverage(vLocalPos, uShape.bounds, uShape.borders, uShape.corners, uShape.type));
}
