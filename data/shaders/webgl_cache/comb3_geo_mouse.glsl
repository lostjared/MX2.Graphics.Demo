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
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

float h1(float n){return fract(sin(n)*43758.5453123);}
vec2 h2(float n){return fract(sin(vec2(n, n+1.0))*vec2(43758.5453,22578.1459));}

vec2 rotateUV(vec2 uv, float a, vec2 c, float aspect) {
    float s = sin(a), cv = cos(a);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cv, -s, s, cv) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float r = length(p);
    float seg = 6.28318530718 / segments;
    ang = mod(ang, seg);
    ang = abs(ang - seg * 0.5);
    vec2 q = vec2(cos(ang), sin(ang)) * r;
    q.x /= aspect;
    return q + c;
}

vec2 tileMirror(vec2 uv, float tiles, vec2 c) {
    vec2 p = (uv - c) * tiles;
    p = abs(fract(p) * 2.0 - 1.0);
    return p / tiles + c;
}

vec2 swirl(vec2 uv, vec2 c, float aspect, float k) {
    vec2 p = uv - c;
    p.x *= aspect;
    float r = length(p) + 1e-6;
    float a = atan(p.y, p.x) + k * r;
    vec2 q = vec2(cos(a), sin(a)) * r;
    q.x /= aspect;
    return q + c;
}

vec2 fractalZoom(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 5; i++) {
        p = abs((p - c) * zoom) - 0.5 + c;
        p = rotateUV(p, t * 0.1, c, aspect);
    }
    return p;
}

void mxCacheShaderMain() {
    float aspect = iResolution.x / iResolution.y;
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = tc;

    float t = time_f;
    float modef = mod(floor(t * 0.1666667), 4.0);
    int mode = int(modef);
    float seed = floor(t * 0.1666667);
    float seg = 4.0 + floor(h1(seed) * 8.0);
    float tiles = 2.0 + floor(h1(seed + 3.1) * 6.0);
    float k = (h1(seed + 7.7) * 2.0 - 1.0) * (0.6 + 0.6 * sin(t * 0.37));
    float zoom = 1.3 + 0.6 * sin(t * 0.5 + h1(seed + 1.7) * 6.2831853);

    vec2 duv = uv;
    if (mode == 0) duv = reflectUV(uv, seg, m, aspect);
    if (mode == 1) duv = tileMirror(rotateUV(uv, 0.4 * sin(t * 0.6), m, aspect), tiles, m);
    if (mode == 2) duv = swirl(uv, m, aspect, k * 6.0);
    if (mode == 3) duv = fractalZoom(uv, zoom, t, m, aspect);

    vec2 d = uv - m;
    float dist = length(d);
    float r = 0.45;
    float w = 1.0 - smoothstep(0.0, r, dist);
    vec2 warp = normalize(d + 1e-5) * sin(dist * (18.0 + seg) - t * (2.0 + h1(seed)*3.0)) * 0.12 * w;
    duv += warp;

    duv = vec2(pingPong(duv.x + 0.05 * sin(t), 1.0), pingPong(duv.y + 0.05 * cos(t), 1.0));

    vec4 base = texture(samp, uv);
    vec4 fx = texture(samp, duv);
    float blend = 0.45 + 0.25 * sin(t * 0.8);
    vec4 mixedCol = mix(base, fx, blend);

    mixedCol.rgb *= 0.5 + 0.5 * sin(duv.xyx * (2.0 + seg * 0.1) + t);

    vec4 c1 = sin(mixedCol * pingPong(t, 10.0));
    vec4 t0 = texture(samp, tc);
    vec4 c2 = sin(c1 * t0 * 0.8 * pingPong(t, 15.0));

    color = c2;
    color.a = 1.0;
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
