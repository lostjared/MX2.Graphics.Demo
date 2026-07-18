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
uniform vec2 iResolution;

uvec3 bytes(vec3 c) {
    return uvec3(clamp(floor(c * 255.0 + 0.5), 0.0, 255.0));
}

vec3 xorColor(vec3 a, vec3 b) {
    return vec3((bytes(a) ^ bytes(b)) & uvec3(255u)) / 255.0;
}

vec3 palette(float t) {
    return 0.55 + 0.45 * cos(6.2831853 * (t + vec3(0.06, 0.38, 0.71)));
}

vec3 crossBlur(vec2 uv, float radius) {
    vec2 px = radius / max(iResolution, vec2(1.0));
    vec3 sum = texture(samp, uv).rgb * 4.0;
    sum += texture(samp, uv + vec2(px.x, 0.0)).rgb;
    sum += texture(samp, uv - vec2(px.x, 0.0)).rgb;
    sum += texture(samp, uv + vec2(0.0, px.y)).rgb;
    sum += texture(samp, uv - vec2(0.0, px.y)).rgb;
    return sum / 8.0;
}

void mxCacheShaderMain() {
    vec3 fine = crossBlur(tc, 1.5);
    vec3 medium = crossBlur(tc, 4.0);
    vec3 wide = crossBlur(tc, 9.0);
    float luminance = dot(medium, vec3(0.299, 0.587, 0.114));
    float mist = 0.5 + 0.5 * sin(tc.x * 4.0 + tc.y * 5.0 + time_f * 0.28);

    float phase = 1.6 + 0.5 * sin(time_f * 0.16);
    vec3 lowBits = xorColor(medium, wide * phase);
    vec3 highBits = xorColor(fine, medium * (phase + 0.35));
    vec3 blendedBits = mix(lowBits, highBits, smoothstep(0.2, 0.8, luminance));
    vec3 tint = palette(luminance * 0.7 + mist * 0.2 + time_f * 0.014);

    vec3 result = mix(fine, blendedBits, 0.25 + mist * 0.16);
    result = mix(result, sqrt(max(result, 0.0)) * tint, 0.32);
    result = mix(result, wide * tint, 0.12);
    color = vec4(clamp(result, 0.0, 1.0), texture(samp, tc).a);
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
