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

vec3 pastel(float t) {
    vec3 vivid = 0.5 + 0.5 * cos(6.2831853 * (t + vec3(0.00, 0.32, 0.66)));
    return mix(vivid, vec3(1.0), 0.34);
}

vec3 bilateral9(vec2 uv) {
    vec2 px = 1.6 / max(iResolution, vec2(1.0));
    vec3 center = texture(samp, uv).rgb;
    vec3 sum = center * 2.5;
    float total = 2.5;
    for (int i = 0; i < 8; ++i) {
        float angle = float(i) * 0.78539816339;
        vec2 offset = vec2(cos(angle), sin(angle)) * px;
        vec3 sampleColor = texture(samp, uv + offset).rgb;
        float weight = exp(-dot(sampleColor - center, sampleColor - center) * 10.0);
        sum += sampleColor * weight;
        total += weight;
    }
    return sum / total;
}

void mxCacheShaderMain() {
    vec2 p = tc - 0.5;
    float flowA = sin(p.y * 9.0 + time_f * 0.65 + sin(p.x * 5.0) * 2.0);
    float flowB = cos(p.x * 8.0 - time_f * 0.52 + sin(p.y * 6.0) * 2.0);
    vec2 flow = vec2(flowA, flowB) * 0.009;

    vec3 base = bilateral9(tc + flow);
    vec3 echo = bilateral9(tc - flow).gbr;
    float luminance = dot(base, vec3(0.299, 0.587, 0.114));
    vec3 bits = xorColor(base, echo * (1.35 + luminance * 0.6));
    vec3 tint = pastel(luminance * 0.55 + (flowA + flowB) * 0.08 + time_f * 0.018);

    vec3 result = mix(base, bits, 0.26);
    result = mix(result, result * tint, 0.34);
    result = mix(result, tint, 0.12);
    result = smoothstep(vec3(-0.05), vec3(1.05), result);
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
