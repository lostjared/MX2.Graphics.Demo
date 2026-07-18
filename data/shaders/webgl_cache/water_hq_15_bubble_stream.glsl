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

in vec2 TexCoord;
#define tc mxCacheTexCoord
out vec4 color;
uniform sampler2D samp;
uniform float time_f;

float hash11(float p) {
    return fract(sin(p * 127.1) * 43758.5453);
}
vec2 safeUV(vec2 p) {
    return 1.0 - abs(mod(p, 2.0) - 1.0);
}

void mxCacheShaderMain() {
    vec2 warp = vec2(0.0);
    float rim = 0.0;
    for (int i = 0; i < 7; i++) {
        float fi = float(i);
        float size = mix(0.025, 0.075, hash11(fi + 2.0));
        vec2 center = vec2(0.12 + 0.76 * hash11(fi + 8.0),
                           fract(hash11(fi + 4.0) + time_f * (0.035 + 0.018 * hash11(fi))));
        center.x += sin(time_f * 0.7 + fi * 2.3) * 0.025;
        vec2 d = tc - center;
        float r = length(d);
        float body = 1.0 - smoothstep(size * 0.82, size, r);
        warp += normalize(d + vec2(1e-5)) * body * (size - r) * 0.24;
        rim += smoothstep(size * 0.68, size * 0.88, r) * (1.0 - smoothstep(size * 0.88, size, r));
    }
    vec4 src = texture(samp, safeUV(tc + warp));
    vec3 rgb = src.rgb + vec3(0.18, 0.29, 0.34) * clamp(rim, 0.0, 1.0) * 0.45;
    color = vec4(rgb, texture(samp, tc).a);
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
