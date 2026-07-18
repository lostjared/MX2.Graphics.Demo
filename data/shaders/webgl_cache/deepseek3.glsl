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

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec3 rainbow(float t) {
    return 0.5 + 0.5 * cos(6.28318 * (t + vec3(0.0, 0.333, 0.666)));
}

vec4 waveEffect(vec2 uv, vec4 original) {
    // Horizontal wave parameters
    float hSpeed = 1.5;
    float hFreq = 2.0;
    float hTime = pingPong(time_f * hSpeed, 2.0);

    // Vertical wave parameters
    float vSpeed = 1.2;
    float vFreq = 1.5;
    float vTime = pingPong(time_f * vSpeed, 2.0);

    // Diagonal wave parameters
    float dSpeed = 2.0;
    float dFreq = 3.0;
    float dTime = pingPong(time_f * dSpeed, 2.0);

    // Create multiple wave patterns
    float wave1 = sin((uv.x * hFreq + hTime) * 6.28318);
    float wave2 = sin((uv.y * vFreq + vTime) * 6.28318);
    float wave3 = sin((uv.x + uv.y) * dFreq + dTime * 3.14159);

    // Combine wave patterns
    float combinedWave = (wave1 + wave2 + wave3) / 3.0;

    // Create color gradient based on wave position
    vec3 waveColor = rainbow(combinedWave * 0.5 + 0.5);

    // Create pulsing alpha based on wave intensity
    float waveAlpha = smoothstep(0.3, 0.7, abs(combinedWave));

    // Create moving wave lines
    float travelingWave =
        sin(uv.x * 20.0 + time_f * 5.0) *
        sin(uv.y * 20.0 - time_f * 3.0) *
        sin(time_f * 2.0);

    // Mix original texture with wave colors
    vec4 finalColor = original;
    finalColor.rgb = mix(
        finalColor.rgb,
        waveColor * (1.0 + travelingWave * 0.5),
        waveAlpha * 0.7
    );

    // Add edge glow
    float edgeGlow = smoothstep(0.8, 0.0, length(uv - 0.5));
    finalColor.rgb += waveColor * edgeGlow * 0.3;

    return finalColor;
}

void mxCacheShaderMain() {
    vec2 uv = tc;

    // Create texture distortion waves
    vec2 offset = vec2(
        sin(time_f + uv.y * 5.0) * 0.02,
        cos(time_f * 0.8 + uv.x * 4.0) * 0.02
    );

    // Get original texture color
    vec4 original = texture(samp, uv + offset);

    // Apply wave effects
    color = waveEffect(uv, original);

    // Add chromatic aberration
    color.r = texture(samp, uv + offset * 0.3).r;
    color.b = texture(samp, uv - offset * 0.3).b;

    color = sin(color * pingPong(time_f, 10.0));

    // Maintain alpha
    color.a = original.a;
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
