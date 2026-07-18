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

void mxCacheShaderMain() {
    float loopDuration = 25.0;
    float t = mod(time_f, loopDuration);
    vec2 aspect = vec2(iResolution.x / iResolution.y, 1.0);
          vec2 uv = 1.0 - abs(1.0 - 2.0 * tc);
    uv = uv - floor(uv);
    vec2 nc = (uv * 2.0 - 1.0) * sin(aspect * time_f);
    nc.x = abs(nc.x);
    float d = length(nc);
    float a = atan(nc.y, nc.x);
    float spiralSpeed = 5.0;
    float inward = t / loopDuration;
    a += (1.0 - smoothstep(0.0, 8.0, d)) * t * spiralSpeed;
    d *= 1.0 - inward;
    vec2 spiral = vec2(cos(a), sin(a)) * tan(d);
    vec2 uv0 = (spiral / aspect + 1.0) * 0.5;

    vec2 p = (uv0 * 2.0 - 1.0) * aspect;
    float r = length(p);
    float ang = atan(p.y, p.x);

    float N = 10.0;
    float tau = 6.28318530718;
    float sector = tau / N;
    ang = mod(ang + 0.5 * sector, sector);
    ang = abs(ang - 0.5 * sector);

    float ringFreq = 6.0;
    float ring = fract(r * ringFreq + 0.15 * sin(time_f * 0.5));
    float ringMirror = abs(ring - 0.5) * 2.0;

    float swirl = 0.25 * sin(time_f * 0.3);
    ang += swirl * r;

    float zoom = 0.85 + 0.1 * sin(time_f * 0.27);
    vec2 m = vec2(cos(ang), sin(ang)) * (r * zoom * (0.85 + 0.15 * ringMirror));

    vec2 uvx = (m / aspect + 1.0) * 0.5;

    vec2 px = 1.0 / iResolution;
    vec3 c = texture(samp, uv).rgb;
    c += texture(samp, uvx + vec2(px.x, 0)).rgb;
    c += texture(samp, uvx + vec2(-px.x, 0)).rgb;
    c += texture(samp, uvx + vec2(0, px.y)).rgb;
    c += texture(samp, uvx + vec2(0, -px.y)).rgb;
    c *= 0.2;

    float glow = smoothstep(0.95, 0.2, r) * (0.6 + 0.4 * ringMirror);
    vec3 base = c * (0.7 + 0.3 * glow);

    float hue = fract(ang / tau + 0.5 + 0.03 * time_f);
    float rC = clamp(abs(hue * 6.0 - 3.0) - 1.0, 0.0, 1.0);
    float gC = clamp(2.0 - abs(hue * 6.0 - 2.0), 0.0, 1.0);
    float bC = clamp(2.0 - abs(hue * 6.0 - 4.0), 0.0, 1.0);
    vec3 rainbow = vec3(rC, gC, bC);

    float rbAmt = 0.35 * smoothstep(0.15, 0.85, ringMirror);
    vec3 outCol = mix(base, base * rainbow, rbAmt);

    float vign = smoothstep(1.2, 0.4, length((tc - 0.5) * aspect));
    outCol *= vign;

    color = vec4(outCol, 1.0);
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
