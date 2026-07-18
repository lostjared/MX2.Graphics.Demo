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
    return 0.52 + 0.48 * cos(6.2831853 * (t + vec3(0.00, 0.31, 0.67)));
}

vec3 softBloom(vec2 uv) {
    vec2 px = 1.5 / max(iResolution, vec2(1.0));
    vec3 sum = texture(samp, uv).rgb * 4.0;
    sum += texture(samp, uv + vec2(px.x, 0.0)).rgb * 2.0;
    sum += texture(samp, uv - vec2(px.x, 0.0)).rgb * 2.0;
    sum += texture(samp, uv + vec2(0.0, px.y)).rgb * 2.0;
    sum += texture(samp, uv - vec2(0.0, px.y)).rgb * 2.0;
    sum += texture(samp, uv + px).rgb;
    sum += texture(samp, uv - px).rgb;
    sum += texture(samp, uv + vec2(px.x, -px.y)).rgb;
    sum += texture(samp, uv + vec2(-px.x, px.y)).rgb;
    return sum / 16.0;
}

void mxCacheShaderMain() {
    vec2 p = tc - 0.5;
    float radius = length(p);
    float angle = atan(p.y, p.x);
    float wave = sin(radius * 42.0 - time_f * 4.0 + angle * 6.0);
    vec2 split = normalize(p + vec2(0.0001)) * (0.002 + 0.006 * wave);

    vec3 prism =
        vec3(texture(samp, tc + split).r, texture(samp, tc).g, texture(samp, tc - split).b);
    vec3 bloom = softBloom(tc);
    vec3 key = palette(radius * 1.8 + angle / 6.2831853 + time_f * 0.04);
    vec3 bits = xorColor(prism, bloom * (1.8 + 0.7 * wave) + key * 0.35);

    float halo = pow(0.5 + 0.5 * wave, 6.0);
    vec3 result = mix(prism, bits, 0.42 + 0.28 * halo);
    result = 1.0 - (1.0 - result) * (1.0 - key * halo * 0.55);
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
