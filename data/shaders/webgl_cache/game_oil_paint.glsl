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

// Soft painterly look using a small Kuwahara-style box average.
out vec4 color;
in vec2 TexCoord;
#define tc mxCacheTexCoord
uniform sampler2D samp;
uniform float time_f;
uniform vec2 iResolution;

vec4 boxAvg(vec2 origin, vec2 px) {
    vec3 sum = vec3(0.0);
    vec3 sumSq = vec3(0.0);
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            vec3 s = texture(samp, origin + vec2(float(x), float(y)) * px).rgb;
            sum += s;
            sumSq += s * s;
        }
    }
    vec3 mean = sum / 9.0;
    vec3 var = abs(sumSq / 9.0 - mean * mean);
    return vec4(mean, var.r + var.g + var.b);
}

void mxCacheShaderMain() {
    vec2 px = 1.5 / iResolution;
    vec4 q0 = boxAvg(tc + vec2(-2.0, -2.0) * px, px);
    vec4 q1 = boxAvg(tc + vec2( 0.0, -2.0) * px, px);
    vec4 q2 = boxAvg(tc + vec2(-2.0,  0.0) * px, px);
    vec4 q3 = boxAvg(tc + vec2( 0.0,  0.0) * px, px);
    vec4 best = q0;
    if (q1.a < best.a) best = q1;
    if (q2.a < best.a) best = q2;
    if (q3.a < best.a) best = q3;
    color = vec4(best.rgb, 1.0);
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
