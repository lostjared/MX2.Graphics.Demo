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

float ramp(float t, float cycle) {
    float phase = mod(t, cycle * 2.0);
    return smoothstep(0.0, 1.0, phase/cycle) -
           smoothstep(1.0, 2.0, phase/cycle);
}

vec3 rainbow(float t) {
    return 0.5 + 0.5 * cos(6.28318 * (t + vec3(0.0, 0.333, 0.666)));
}

void mxCacheShaderMain() {
    vec2 uv = tc;

    // Wave timing parameters (6 second cycles)
    float hCycle = 6.0;
    float vCycle = 5.5;
    float dCycle = 6.5;

    // Horizontal wave movement
    float hPhase = time_f * 0.5;
    float hWave = ramp(hPhase, hCycle) * 2.0 - 1.0;

    // Vertical wave movement
    float vPhase = time_f * 0.45;
    float vWave = ramp(vPhase, vCycle) * 2.0 - 1.0;

    // Diagonal wave movement
    float dPhase = time_f * 0.6;
    float dWave = ramp(dPhase, dCycle) * 2.0 - 1.0;

    // Create sustained wave patterns
    float waveX = sin(uv.x * 8.0 + hWave * 20.0);
    float waveY = sin(uv.y * 6.0 + vWave * 15.0);
    float waveD = sin((uv.x + uv.y) * 10.0 + dWave * 25.0);

    // Combine waves with different frequencies
    float combined = (waveX + waveY + waveD) / 3.0;

    // Create color gradient
    vec3 waveColor = rainbow(combined * 0.5 + 0.5 + time_f * 0.05);

    // Texture manipulation
    vec2 distort = vec2(
        combined * 0.02 * hWave,
        combined * 0.02 * vWave
    );

    vec4 tex = texture(samp, uv + distort);

    // Create moving color bands
    float colorBand = smoothstep(0.3, 0.7,
        sin(uv.x * 3.0 - hPhase * 0.5) *
        sin(uv.y * 2.0 + vPhase * 0.3) *
        sin((uv.x - uv.y) * 4.0 + dPhase * 0.4)
    );

    // Blend with original texture
    color = mix(tex, vec4(waveColor, 1.0), colorBand * 0.6);

    // Add directional glow
    vec2 flowDir = normalize(vec2(hWave, vWave));
    float flowMask = dot(uv - 0.5, flowDir);
    color.rgb += waveColor * smoothstep(-0.5, 0.5, flowMask) * 0.2;

    // Chromatic movement
    color.r = texture(samp, uv + distort * 0.3).r;
    color.b = texture(samp, uv - distort * 0.3).b;

    color = sin(color * pingPong(time_f, 25.0));
    // Maintain original alpha
    color.a = tex.a;
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
