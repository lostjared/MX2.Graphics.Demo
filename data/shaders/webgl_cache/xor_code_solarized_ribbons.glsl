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

vec3 fromBytes(uvec3 c) {
    return vec3(c & uvec3(255u)) / 255.0;
}

vec3 palette(float t) {
    return 0.54 + 0.46 * cos(6.2831853 * (t + vec3(0.04, 0.30, 0.61)));
}

void mxCacheShaderMain() {
    float aspect = max(iResolution.x, 1.0) / max(iResolution.y, 1.0);
    vec2 p = (tc - 0.5) * vec2(aspect, 1.0);
    float ribbonA = sin(p.y * 15.0 + sin(p.x * 5.0 - time_f) * 3.0 + time_f * 2.0);
    float ribbonB = cos(p.x * 17.0 + sin(p.y * 7.0 + time_f) * 2.5 - time_f * 1.7);
    float ribbons = ribbonA * ribbonB;
    vec2 flow = vec2(ribbonA, ribbonB) * 0.018;

    vec3 base = texture(samp, tc + flow).rgb;
    uvec3 src = bytes(base);
    uvec3 inverted = uvec3(255u) - src;
    uvec3 ribbonMask =
        uvec3(uint((ribbonA * 0.5 + 0.5) * 255.0), uint((ribbonB * 0.5 + 0.5) * 255.0),
              uint((ribbons * 0.5 + 0.5) * 255.0));
    vec3 solar = fromBytes((src ^ ribbonMask) ^ (inverted >> uvec3(1u, 2u, 3u)));

    vec3 tint = palette(ribbons * 0.22 + p.x * 0.08 + time_f * 0.03);
    float crest = pow(abs(ribbons), 7.0);
    vec3 result = mix(base, solar, 0.48 + 0.22 * abs(ribbons));
    result = mix(result, result * tint * 1.35, 0.36);
    result += tint * crest * 0.35;
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
