#version 300 es
precision highp float;
precision highp int;
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

vec2 mxCacheApplyCoordinateAdjustments(vec2 uv, float frequency, float zoom,
                                       float rotation, float quality, vec2 resolution) {
    vec2 p = uv - vec2(0.5);
    float c = cos(rotation);
    float s = sin(rotation);
    p = mat2(c, -s, s, c) * p;
    p *= max(frequency, 0.0);
    p /= max(abs(zoom), 0.001);
    uv = p + vec2(0.5);
    if (quality < 1.0) {
        vec2 grid = max(resolution * max(quality, 0.05), vec2(1.0));
        uv = (floor(uv * grid) + vec2(0.5)) / grid;
    }
    return uv;
}

vec3 mxCacheRotateHue(vec3 col, float angle) {
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

vec3 mxCacheApplyColorAdjustments(vec3 col, float brightness, float contrast,
                                  float saturation, float hueShift) {
    col *= brightness;
    col = (col - 0.5) * contrast + 0.5;
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(gray), col, saturation);
    return mxCacheRotateHue(col, hueShift);
}

vec2 mxCacheTexCoord;

out vec4 color;
in vec2 TexCoord;
#define tc mxCacheTexCoord

uniform sampler2D samp;
uniform float time_f;
uniform vec2 iResolution;

const float PI = 3.14159265359;

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec2 polarDistort(vec2 uv) {
    vec2 center = uv - 0.5;
    float angle = atan(center.y, center.x);
    float radius = length(center) * 2.0;

    // Create fractal layers
    float fractal = sin(angle * 5.0 + time_f) * 0.2;
    fractal += sin(angle * 10.0 + time_f * 2.0) * 0.1;
    fractal += sin(angle * 20.0 + time_f * 0.5) * 0.05;

    radius += fractal * 0.3;
    angle += sin(time_f + radius * 5.0) * 0.5;

    vec2 distorted = vec2(cos(angle), sin(angle)) * radius;
    return distorted + 0.5;
}

vec4 fractalColor(vec2 uv) {
    // Create multiple displacement layers
    vec2 uv1 = uv + vec2(sin(time_f * 0.7 + uv.y * 5.0), cos(time_f * 0.6 + uv.x * 5.0)) * 0.1;
    vec2 uv2 = uv + vec2(cos(time_f * 0.5 + uv.y * 10.0), sin(time_f * 0.4 + uv.x * 10.0)) * 0.05;
    vec2 uv3 = uv + vec2(sin(time_f * 0.3 + uv.y * 20.0), cos(time_f * 0.2 + uv.x * 20.0)) * 0.025;

    // Combine texture samples with color modulation
    vec4 col1 = texture(samp, uv1);
    vec4 col2 = texture(samp, uv2);
    vec4 col3 = texture(samp, uv3);

    // Create color shifting effect
    return vec4(col1.r, col2.g, col3.b, 1.0) * (1.0 + sin(time_f * 2.0) * 0.3);
}

void mxCacheShaderMain() {
    vec2 uv = tc;

    // Create fractal coordinate system
    uv = polarDistort(uv);

    // Add multiple warping layers
    uv.x += sin(uv.y * 10.0 + time_f * 2.0) * 0.03;
    uv.y += cos(uv.x * 8.0 + time_f * 1.5) * 0.03;

    // Create geometric patterns
    float fractalScale = 5.0;
    uv.x += sin(uv.y * fractalScale + time_f) * 0.1;
    uv.y += cos(uv.x * fractalScale + time_f) * 0.1;

    // Ping pong effect with multiple layers
    float layer1 = pingPong(uv.x + time_f * 0.1, 1.0);
    float layer2 = pingPong(uv.y + time_f * 0.15, 1.0);
    uv = mix(uv, vec2(layer1, layer2), 0.3);

    // Final color with texture preservation
    vec4 finalColor = fractalColor(uv);
    vec4 originalColor = texture(samp, tc);

    // Mix between warped and original texture based on radius
    vec2 centerVec = tc - 0.5;
    float radius = length(centerVec) * 2.0;
    float mixFactor = smoothstep(0.3, 0.7, radius);
    finalColor = mix(finalColor, originalColor, mixFactor * 0.5);

    color = finalColor;
}
void main() {
    mxCacheTexCoord = mxCacheApplyCoordinateAdjustments(
        TexCoord, iFrequency, iZoom, iRotation, iQuality, vec2(textureSize(samp, 0)));
    vec4 mxCacheInputColor = texture(samp, mxCacheTexCoord);
    mxCacheShaderMain();
    vec4 mxCacheEffectColor = color;
    mxCacheEffectColor = mix(mxCacheInputColor, mxCacheEffectColor, iAmplitude);
    mxCacheEffectColor.rgb = mxCacheApplyColorAdjustments(
        mxCacheEffectColor.rgb, iBrightness, iContrast, iSaturation, iHueShift);
    if (iDebugMode > 0.5 && TexCoord.x < 0.5) {
        mxCacheEffectColor = mxCacheInputColor;
    }
    color = mxCacheEffectColor;
}
