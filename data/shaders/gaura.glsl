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

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}



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
    vec4 texColor = texture(textTexture, uv);

    float pulse = 0.5 + 0.5 * sin(time * 3.0);

    vec2 offset1 = vec2(0.01, 0.02) * sin(time + length(uv - 0.5) * 20.0);
    vec2 offset2 = vec2(-0.02, 0.01) * cos(time * 1.5 + length(uv - 0.5) * 15.0);
    vec2 offset3 = vec2(0.03, -0.03) * sin(time * 5.0);

    vec3 aura1 = texture(textTexture, uv + offset1).rgb * 0.6;
    vec3 aura2 = texture(textTexture, uv + offset2).rgb * 0.4;
    vec3 aura3 = texture(textTexture, uv + offset3).rgb * 0.3;

    vec3 strobingGradient = vec3(
        0.5 + 0.5 * sin(time + uv.x * 10.0 * iFrequency),
        0.5 + 0.5 * cos(time + uv.y * 10.0),
        0.5 + 0.5 * sin(time + (uv.x + uv.y) * 10.0)
    );

    vec3 auraGlow = mix(aura1 + aura2 + aura3, strobingGradient, 0.5);
    float auraStrength = 0.4 + 0.3 * sin(time * 4.0 + length(uv - 0.5) * 10.0);

    vec3 finalAura = auraGlow * auraStrength;
    FragColor = vec4(sin(finalAura * (pingPong(time, 8.0) + 2.0)), 0.5 * texColor.a);
}

