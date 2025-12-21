#version 300 es
precision highp float;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform float iSpeed;
uniform float iAmplitude;
uniform float iFrequency;
uniform float iBrightness;
uniform float iContrast;
uniform float iSaturation;
uniform float iHueShift;
uniform float iZoom;
uniform float iRotation;
uniform float iQuality;
uniform float iDebugMode;

out vec4 FragColor;


vec3 adjustBrightness(vec3 col, float b) {
    return col * b;
}

vec3 adjustContrast(vec3 col, float c) {
    return (col - 0.5) * c + 0.5;
}

vec3 adjustSaturation(vec3 col, float s) {
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    return mix(vec3(gray), col, s);
}

vec3 rotateHue(vec3 col, float angle) {
    float U = cos(angle);
    float W = sin(angle);
    mat3 R = mat3(
        0.299 + 0.701*U + 0.168*W,
        0.587 - 0.587*U + 0.330*W,
        0.114 - 0.114*U - 0.497*W,
        0.299 - 0.299*U - 0.328*W,
        0.587 + 0.413*U + 0.035*W,
        0.114 - 0.114*U + 0.292*W,
        0.299 - 0.300*U + 1.250*W,
        0.587 - 0.588*U - 1.050*W,
        0.114 + 0.886*U - 0.203*W
    );
    return clamp(R * col, 0.0, 1.0);
}

vec3 applyColorAdjustments(vec3 col) {
    col = adjustBrightness(col, iBrightness);
    col = adjustContrast(col, iContrast);
    col = adjustSaturation(col, iSaturation);
    col = rotateHue(col, iHueShift);
    return clamp(col, 0.0, 1.0);
}

vec2 applyZoomRotation(vec2 uv, vec2 center) {
    vec2 p = uv - center;
    float c = cos(iRotation);
    float s = sin(iRotation);
    p = mat2(c, -s, s, c) * p;
    float z = max(abs(iZoom), 0.001);
    p /= z;
    return p + center;
}

void main() {
    float time = time_f * iSpeed;
    vec2 uv = gl_FragCoord.xy / iResolution;
    vec2 centeredUV = uv * 2.0 - 1.0;

    float speedFactorX = sin(time * 0.5 * iAmplitude) * 0.5 + 0.5;
    float speedFactorY = cos(time * 0.5) * 0.5 + 0.5;

    float angleX = time * (0.2 + speedFactorX * 0.8);
    float angleY = time * (0.2 + speedFactorY * 0.8);

    centeredUV.x = abs(centeredUV.x);
    centeredUV.y = abs(centeredUV.y);

    mat2 rotationX = mat2(cos(angleX), -sin(angleX), sin(angleX), cos(angleX));
    mat2 rotationY = mat2(cos(angleY), -sin(angleY), sin(angleY), cos(angleY));

    vec2 rotatedUV = centeredUV;
    rotatedUV.x = (rotationX * vec2(rotatedUV.x, 0.0)).x;
    rotatedUV.y = (rotationY * vec2(0.0, rotatedUV.y)).y;

    vec2 mirroredUV = mod(rotatedUV, 1.0);
    vec4 texColor = texture(textTexture, mirroredUV);

    float pulse = 0.5 + 0.5 * sin(time * 3.0);
    vec3 pulsatingColor = vec3(
        0.5 + 0.5 * sin(time + uv.x * 10.0 * iFrequency),
        0.5 + 0.5 * cos(time + uv.y * 10.0),
        0.5 + 0.5 * sin(time + (uv.x + uv.y) * 10.0)
    ) * pulse;

    FragColor = vec4(texColor.rgb * 0.7 + pulsatingColor * 0.3, texColor.a);
}

