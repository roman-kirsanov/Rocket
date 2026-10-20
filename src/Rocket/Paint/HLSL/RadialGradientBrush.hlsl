/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "Painter.hlsli"

ConstantBuffer<StateUniforms>         uState : register(b0, space3);
ConstantBuffer<GradientBrushUniforms> uBrush : register(b1, space3);
ConstantBuffer<ShapeUniforms>         uShape : register(b2, space3);

float _stopPosition(int i) {
    return uBrush.stopPositions[i / 4][i % 4];
}

float4 main(QuadVaryings input) : SV_Target {
    float4 result       = float4(0.0, 0.0, 0.0, 0.0);
    float  lineDistance = distance(uBrush.startPoint, uBrush.stopPoint);
    float  svexDistance = distance(uBrush.startPoint, input.localUV);

    if (svexDistance < lineDistance) {
        float t = (svexDistance / lineDistance);
        for (int i = 0; i < (uBrush.stopCount + 1); i++) {
            float tLeft  = (i > 0                ? _stopPosition(i - 1) : 0.0);
            float tRight = (i < uBrush.stopCount ? _stopPosition(i)     : 1.0);
            if (t >= tLeft && t < tRight) {
                float4 leftColor  = (i > 0                ? uBrush.stopColors[i - 1] : uBrush.startColor);
                float4 rightColor = (i < uBrush.stopCount ? uBrush.stopColors[i]     : uBrush.stopColor);
                float tt = ((t - tLeft) / (tRight - tLeft));
                result = lerp(leftColor, rightColor, tt);
                break;
            }
        }
    } else {
        result = uBrush.stopColor;
    }

    return (result * uState.opacity * _coverage(input.localPos, uShape.bounds, uShape.borders, uShape.corners, uShape.type));
}
