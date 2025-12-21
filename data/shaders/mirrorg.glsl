#version 300 es
precision highp float;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
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
    vec2 centeredUV = (uv - 0.5) * 2.0;

    float radius = length(centeredUV);
    float mask = smoothstep(1.0, 0.9, radius);

    float angle = atan(centeredUV.y, centeredUV.x) + time * 0.3;
    float refractAmount = pow(1.0 - radius, 2.0) * 0.2 * iAmplitude;

    vec2 refractedUV = vec2(
        centeredUV.x + sin(angle) * refractAmount,
        centeredUV.y + cos(angle) * refractAmount
    );

    vec2 crystalUV = refractedUV * 0.5 + 0.5;
    vec4 texColorBall = texture(textTexture, crystalUV);
    vec4 texColorBackground = texture(textTexture, uv);

    vec4 mixResult = mix(texColorBackground, texColorBall, mask);
    vec3 col = applyColorAdjustments(mixResult.rgb);
    FragColor = vec4(col, mixResult.a);
}
