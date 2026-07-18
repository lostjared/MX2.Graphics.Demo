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
uniform float time_f;
uniform vec2 iResolution;
uniform sampler2D samp;

float PI = 3.1415926535897932384626433832795;

void mxCacheShaderMain() {
    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);
    vec2 p = (tc - 0.5) * ar;

    float r = max(length(p), 1e-4);
    float a = atan(p.y, p.x);

    float t = time_f;
    float pulse = 0.5 + 0.5 * sin(t * 2.0);
    float zoomSpeed = 0.35 + 0.15 * sin(t * 0.7);
    float swirl = 0.05 * sin(6.0 * a + t * 2.0 + r * 10.0);
    float wobble = 0.10 * sin(a * 8.0 + t * 1.5);

    float lr = log(r);
    lr += zoomSpeed * t;
    lr = fract(lr * (1.0 + 0.25 * pulse));
    lr = (lr - 0.5) * 2.0 + wobble;

    a += swirl;

    float nr = exp(lr);
    vec2 q = vec2(cos(a), sin(a)) * nr;
    vec2 baseUV = q / ar + 0.5;

    vec2 uv = (tc - 0.5) * 2.0;
    uv.x *= ar.x;

    float plasma = 0.0;
    plasma += sin((uv.x + t) * 5.0);
    plasma += sin((uv.y + t) * 5.0);
    plasma += sin((uv.x + uv.y + t) * 5.0);
    plasma += cos(length(uv + t) * 10.0);
    plasma *= 0.25;

    vec3 baseColor;
    baseColor.r = cos(plasma * PI + t * 0.2) * 0.5 + 0.5;
    baseColor.g = sin(plasma * PI + t * 0.2) * 0.5 + 0.5;
    baseColor.b = sin(plasma * PI + t * 0.4) * 0.5 + 0.5;

    vec2 dir = normalize(q + 1e-5);
    float disp = 0.002 + 0.01 * pulse;

    float rC = texture(samp, baseUV + dir * disp).r;
    float gC = texture(samp, baseUV).g;
    float bC = texture(samp, baseUV - dir * disp).b;

    vec3 prismColor = vec3(rC, gC, bC);

    float morph = 0.5 + 0.5 * sin(t * 1.1);
    vec3 mixed = mix(baseColor, prismColor, 0.6 + 0.4 * morph);

    float breathe = 0.5 + 0.5 * sin(t * 3.0 + length(q) * 12.0);
    mixed *= 0.9 + 0.2 * breathe;

    color = vec4(mixed, 1.0);
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
