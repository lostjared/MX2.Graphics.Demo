#ifndef _SHADERS_HPP
#define _SHADERS_HPP

#define COMMON_UNIFORMS R"(
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
)"

// Helper functions for color adjustments
#define COLOR_HELPERS R"(
vec3 adjustBrightness(vec3 col, float b) {
    return col * b;
}

vec3 adjustContrast(vec3 col, float c) {
    return (col - 0.5) * c + 0.5;
}

vec3 adjustSaturation(vec3 col, float s) {
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    return mix(vec3(gray), col, s);
}

vec3 rotateHue(vec3 col, float angle) {
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

vec3 applyColorAdjustments(vec3 col) {
    col = adjustBrightness(col, iBrightness);
    col = adjustContrast(col, iContrast);
    col = adjustSaturation(col, iSaturation);
    col = rotateHue(col, iHueShift);
    return clamp(col, 0.0, 1.0);
}

vec2 applyZoomRotation(vec2 uv, vec2 center) {
    vec2 p = uv - center;
    float c = cos(iRotation);
    float s = sin(iRotation);
    p = mat2(c, -s, s, c) * p;
    p /= iZoom;
    return p + center;
}
)"

inline const char *srcShader1 = R"(#version 300 es
precision highp float;
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

void main(void) {
    float time = time_f * iSpeed;
    vec2 uv = applyZoomRotation(TexCoord, vec2(0.5));
    uv = uv * 2.0 - 1.0;
    float len = length(uv);
    float bubble = smoothstep(0.8, 1.0, 1.0 - len);
    float distortAmt = 0.1 * iAmplitude;
    vec2 distort = uv * (1.0 + distortAmt * sin(time + len * 20.0 * iFrequency));
    vec4 texColor = texture(textTexture, distort * 0.5 + 0.5);
    vec3 col = mix(texColor.rgb, vec3(1.0), bubble);
    col = applyColorAdjustments(col);
    FragColor = vec4(col, texColor.a);
}
)";

inline const char *srcShader3 = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    int iterations = int(3.0 + iQuality * 3.0);
    for (int i = 0; i < 6; i++) {
        if (i >= iterations) break;
        p = abs((p - c) * (zoom + 0.15 * iAmplitude * sin(t * 0.35 * iFrequency + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f * iSpeed;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec4 baseTex = texture(textTexture, tc);
    vec2 uv = tc * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    vec2 m = (iMouse.z > 0.5) ? vec2(iMouse.x / iResolution.x, 1.0 - iMouse.y / iResolution.y) : vec2(0.5);
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(tc);
    float seg = 4.0 + 2.0 * sin(time * 0.33 * iFrequency);
    vec2 kUV = reflectUV(tc, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * iAmplitude * sin(time * 0.42);
    kUV = fractalFold(kUV, foldZoom, time, m, aspect);
    kUV = rotateUV(kUV, time * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time * 0.2) * (PI * time), 5.0);
    float period = log(base) * pingPong(time * PI, 5.0);
    float tz = pingPong(time * PI/2.0, 3.0) * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 * iFrequency + pingPong(time * PI/2.0, 3.0) * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((tc - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + pingPong(time * PI, 5.0) * 1.2));
    ring = ring * pingPong((time * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(pingPong(time * PI, 5.0) * 2.0 + rD * 28.0 + k * 12.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    
    finalRGB = applyColorAdjustments(finalRGB);
    
    // Debug mode visualization
    if (iDebugMode > 0.5) {
        finalRGB = vec3(tc, 0.5);
    }
    
    color = vec4(finalRGB, baseTex.a);
}
)";

inline const char *srcShader4 = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 uv = tc;
    float time_t = pingPong(time, 10.0);
    float scrambleAmount = sin(time * 10.0 * iFrequency) * 0.5 + 0.5;
    scrambleAmount = cos(scrambleAmount * time_t) * iAmplitude;
    uv.x += sin(uv.y * 50.0 * iFrequency + time * 10.0) * scrambleAmount * 0.05;
    uv.y += cos(uv.x * 50.0 * iFrequency + time * 10.0) * scrambleAmount * 0.05;
    vec4 texColor = texture(textTexture, tan(uv * time_t));
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *srcShader5 = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform float alpha;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

float smoothNoise(vec2 uv, float time) {
    return sin(uv.x * 12.0 * iFrequency + uv.y * 14.0 * iFrequency + time * 0.8) * 0.5 + 0.5;
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 uv = (tc * iResolution - 0.5 * iResolution) / iResolution.y;
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);
    float swirl = sin(time * 0.4) * 2.0 * iAmplitude;
    angle += swirl * radius * 1.5;
    float modRadius = pingPong(radius + time * 0.3, 0.8);
    float wave = sin(radius * 12.0 * iFrequency - time * 3.0) * 0.5 + 0.5;
    float cloudNoise = smoothNoise(uv * 3.0 + vec2(modRadius, angle * 0.5), time);
    cloudNoise += smoothNoise(uv * 6.0 - vec2(time * 0.2, time * 0.1), time);
    cloudNoise = pow(cloudNoise, 1.5);
    float r = sin(angle * 3.0 + modRadius * 8.0 + wave * 2.0) * cloudNoise;
    float g = sin(angle * 5.0 - modRadius * 6.0 + wave * 4.0) * cloudNoise;
    float b = sin(angle * 7.0 + modRadius * 10.0 - wave * 3.0) * cloudNoise;
    vec3 col = vec3(r, g, b) * 0.5 + 0.5;
    vec3 texColor = texture(textTexture, tc).rgb;
    col = mix(col, texColor, 0.25);
    col = sin(col * pingPong(time * 1.5, 6.0) + 1.5);
    col = applyColorAdjustments(col);
    color = vec4(col, alpha);
}
)";

inline const char *srcShader2 = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    int iterations = int(3.0 + iQuality * 3.0);
    for (int i = 0; i < 6; i++) {
        if (i >= iterations) break;
        p = abs((p - c) * (zoom + 0.15 * iAmplitude * sin(t * 0.35 * iFrequency + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv, float time) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    vec3 neon = neonPalette(time + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec4 baseTex = texture(textTexture, tc);
    vec2 uv = tc * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    vec2 m = (iMouse.z > 0.5) ? vec2(iMouse.x / iResolution.x, 1.0 - iMouse.y / iResolution.y) : vec2(0.5);
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(tc, time);
    float seg = 4.0 + 2.0 * sin(time * 0.33 * iFrequency);
    vec2 kUV = reflectUV(tc, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * iAmplitude * sin(time * 0.42);
    kUV = fractalFold(kUV, foldZoom, time, m, aspect);
    kUV = rotateUV(kUV, time * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time * 0.2) * (PI * time), 5.0);
    float period = log(base) * pingPong(time * PI, 5.0);
    float tz = pingPong(time * PI/2.0, 3.0) * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 * iFrequency + pingPong(time * PI/2.0, 3.0) * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((tc - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off, time);
    vec3 gC = preBlendColor(u1, time);
    vec3 bC = preBlendColor(u2 - off, time);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + pingPong(time * PI, 5.0) * 1.2));
    ring = ring * pingPong((time * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(pingPong(time * PI, 5.0) * 2.0 + rD * 28.0 + k * 12.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    finalRGB = applyColorAdjustments(finalRGB);
    color = vec4(finalRGB, baseTex.a);
}
)";

inline const char *szShader = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform float time_f;
uniform sampler2D textTexture;
uniform vec2 iResolution;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 uv = tc;

    vec2 reflectedUV = vec2(1.0 - uv.x, uv.y);
    float noise = sin(reflectedUV.y * 50.0 * iFrequency + time * 5.0) * 0.01 * iAmplitude;
    noise = sin(noise * pingPong(time * PI, 6.0));
    float distortionX = noise;
    float distortionY = cos(reflectedUV.x * 50.0 * iFrequency + time * 5.0) * 0.01 * iAmplitude;
    vec2 distortion = vec2(distortionX, distortionY);
    float r = texture(textTexture, reflectedUV + distortion * 1.2).r;
    float g = texture(textTexture, reflectedUV + distortion * 0.8).g;
    float b = texture(textTexture, reflectedUV + distortion * 0.4).b;
    vec3 col = applyColorAdjustments(vec3(r, g, b));
    color = vec4(col, 1.0);
}
)";

inline const char *szShader2 = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;

uniform float time_f;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 uv = tc;
    float rollSpeed = 0.2 * iSpeed;
    uv.y = mod(uv.y + time * rollSpeed, 1.0);
    float bendAmplitude = 0.02 * iAmplitude;
    float bendFrequency = 10.0 * iFrequency;
    uv.x += sin(uv.y * 3.1415 * bendFrequency + time * 2.0) * bendAmplitude;
    float jitterChance = rand(vec2(time, uv.y));
    if(jitterChance > 0.95) {
        float jitterAmount = (rand(vec2(time * 2.0, uv.y * 3.0)) - 0.5) * 0.1 * iAmplitude;
        uv.x += jitterAmount;
    }
    float chromaShift = 0.005 * iAmplitude;
    float r = texture(textTexture, uv + vec2(chromaShift, 0.0)).r;
    float g = texture(textTexture, uv).g;
    float b = texture(textTexture, uv - vec2(chromaShift, 0.0)).b;
    vec3 col = applyColorAdjustments(vec3(r, g, b));
    color = vec4(col, 1.0);
}
)";

inline const char *szTwist = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture;
uniform float time_f;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float rippleSpeed = 5.0 * iSpeed;
    float rippleAmplitude = 0.03 * iAmplitude;
    float rippleWavelength = 10.0 * iFrequency;
    float twistStrength = 1.0 * iAmplitude;
    float radius = length(tc - vec2(0.5, 0.5));
    float ripple = sin(tc.x * rippleWavelength + time * rippleSpeed) * rippleAmplitude;
    ripple += sin(tc.y * rippleWavelength + time * rippleSpeed) * rippleAmplitude;
    vec2 rippleTC = tc + vec2(ripple, ripple);
    
    float angle = twistStrength * (radius - 1.0) + time;
    float cosA = cos(angle);
    float sinA = sin(angle);
    mat2 rotationMatrix = mat2(cosA, -sinA, sinA, cosA);
    vec2 twistedTC = (rotationMatrix * (tc - vec2(0.5, 0.5))) + vec2(0.5, 0.5);
    
    vec4 twistedRippleColor = texture(textTexture, mix(rippleTC, twistedTC, 0.5));
    twistedRippleColor.rgb = applyColorAdjustments(twistedRippleColor.rgb);
    color = twistedRippleColor;
}
)";

inline const char *szTime = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

float h1(float n){ return fract(sin(n)*43758.5453123); }
vec2 h2(float n){ return fract(sin(vec2(n, n+1.0))*vec2(43758.5453,22578.1459)); }

void main(void){
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float rate = 0.6;
    float t = time * rate;
    float t0 = floor(t);
    float a = fract(t);
    float w = a*a*(3.0-2.0*a);
    vec2 p0 = vec2(0.15) + h2(t0)*0.7;
    vec2 p1 = vec2(0.15) + h2(t0+1.0)*0.7;
    vec2 center = mix(p0, p1, w);

    vec2 p = tc - center;
    float r = length(p);
    float ang = atan(p.y, p.x);

    float swirl = (2.2 + 0.8*sin(time*0.35)) * iAmplitude;
    float spin = 0.6*sin(time*0.2);
    ang += swirl * r + spin;

    float bend = 0.35 * iAmplitude;
    float rp = r + bend * r * r;

    vec2 uv = center + vec2(cos(ang), sin(ang)) * rp;

    uv += 0.02 * iAmplitude * vec2(sin((tc.y+time)*4.0*iFrequency), cos((tc.x-time)*3.5*iFrequency));

    uv = vec2(pingPong(uv.x, 1.0), pingPong(uv.y, 1.0));

    vec4 texColor = texture(textTexture, uv);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *szBend = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 uv = tc;
    float t = mod(time, 10.0) * 0.1;
    float angle = sin(t * 3.14159265) * -0.5;
    vec2 center = vec2(0.5, 0.5);
    uv -= center;
    float dist = length(uv);
    float bend = sin(dist * 6.0 * iFrequency + t * 2.0 * 3.14159265) * 0.05 * iAmplitude;
    uv = mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * uv;
    uv += center;
    float time_t = pingPong(time, 10.0);
    uv -= sin(bend * uv * time_t);
    vec4 texColor = texture(textTexture, uv);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
}
)";

inline const char *szPong = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float len){
    float m = mod(x, len*2.0);
    return m <= len ? m : len*2.0 - m;
}

void main(void){
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (tc - 0.5) * ar;
    float base = 1.65;
    float period = log(base);
    float t = time * 0.45;
    float r = length(p) + 1e-6;
    float a = atan(p.y, p.x);
    float k = fract((log(r) - t) / period);
    float rw = exp(k * period);
    a += t * 0.4;
    vec2 z = vec2(cos(a), sin(a)) * rw;
    float swirl = sin(time * 0.6 + r * 5.0 * iFrequency) * 0.1 * iAmplitude;
    z += swirl * normalize(p);
    vec2 uv = z / ar + 0.5;
    uv = fract(uv);
    vec4 tex = texture(textTexture, uv);
    tex.rgb = applyColorAdjustments(tex.rgb);
    color = tex;
}
)";

inline const char *psychWave = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform float time_f;
uniform sampler2D textTexture;
uniform vec2 iResolution;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

void main(void)
{
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 normCoord = ((gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
    float distanceFromCenter = length(normCoord);
    float wave = sin(distanceFromCenter * 12.0 * iFrequency - time * 4.0) * iAmplitude;
    vec2 tcAdjusted = tc + (normCoord * 0.301 * wave);
    vec4 textureColor = texture(textTexture, tcAdjusted);
    textureColor.rgb = applyColorAdjustments(textureColor.rgb);
    color = textureColor;
}
)";

inline const char *crystalBall = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float rand1(float x) { return fract(sin(x) * 43758.5453); }

vec3 brutalColorDistortion(vec3 col, vec2 uv, float t) {
    float distAmt = 0.03 * iAmplitude;
    col.r = texture(textTexture, uv + vec2(sin(t*0.7)*distAmt, cos(t*0.3)*distAmt*0.67)).r;
    col.g = texture(textTexture, uv + vec2(sin(t*0.5)*distAmt*0.67, cos(t*0.4)*distAmt)).g;
    col.b = texture(textTexture, uv + vec2(sin(t*0.9)*distAmt*1.33, cos(t*0.6)*distAmt*0.33)).b;
    
    if (mod(t, 3.0) > 2.0) col = col.bgr;
    if (mod(t*0.7, 2.0) > 1.3) col = col.grb;
    
    return col;
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 uv = tc;
    float t = time * 3.0;
    
    float distortAmt = 0.1 * iAmplitude;
    uv.x += sin(uv.y * 50.0 * iFrequency + t * 10.0) * distortAmt * rand1(t);
    uv.y += cos(uv.x * 40.0 * iFrequency + t * 8.0) * distortAmt * 0.8 * rand1(t+0.3);
    
    uv.x += sin(t * 5.0 + uv.y * 50.0 * iFrequency) * 0.05 * iAmplitude;
    uv.y += cos(t * 3.0 + uv.x * 60.0 * iFrequency) * 0.03 * iAmplitude;
    
    uv += vec2(rand1(t) - 0.5, rand1(t+0.5) - 0.5) * 0.1 * iAmplitude;
    
    vec2 crtUV = uv * 2.0 - 1.0;
    crtUV *= 1.0 + pow(length(crtUV), 3.0) * 0.5 * iAmplitude;
    uv = crtUV * 0.5 + 0.5;
    
    float split = step(0.5 + sin(t)*0.2, uv.x);
    uv.y += split * (sin(t*2.0) * 0.1 * rand1(t)) * iAmplitude;
    
    float scanLine = fract(t * 0.3);
    if(abs(uv.y - scanLine) < 0.02) {
        uv.x += (rand1(uv.y + t) - 0.5) * 0.3 * iAmplitude;
        uv.y += sin(uv.x * 100.0 * iFrequency) * 0.1 * iAmplitude;
    }
    
    vec3 col = brutalColorDistortion(texture(textTexture, uv).rgb, uv, t);
    
    float damageZone = step(0.9, rand(vec2(floor(uv.y * 20.0 + t * 0.5))));
    col = mix(col, vec3(0.0), damageZone * 0.7);
    
    float staticFlash = step(0.997, rand1(t * 0.1));
    col += vec3(staticFlash * 2.0);
    
    col *= 0.7 + 0.3 * sin(uv.x * 300.0 * iFrequency + t * 10.0);
    
    vec3 noise = vec3(rand(uv * t), rand(uv * t + 0.3), rand(uv * t + 0.7)) * 0.8;
    col += noise * smoothstep(0.3, 0.7, rand1(t * 0.3)) * iAmplitude;
    
    float crease = sin(uv.y * 30.0 * iFrequency + t * 5.0) * 0.1;
    col *= 1.0 - smoothstep(0.3, 0.7, abs(crease));
    
    col.r += sin(t * 5.0) * 0.3 + rand1(uv.x) * 0.2;
    
    float osd = step(0.98, rand1(uv.x * 10.0 + t * 0.1));
    col = mix(col, vec3(0.0, 1.0, 0.0), osd * 0.3);
    
    col = applyColorAdjustments(col);
    
    color = vec4(col, 1.0);
    color *= 0.8 + 0.2 * sin(uv.y * 800.0 + t * 10.0);
    color *= 0.7 + 0.3 * rand1(t * 0.1);
    color *= smoothstep(0.0, 0.2, uv.y) * smoothstep(1.0, 0.8, uv.y);
    color *= smoothstep(0.0, 0.1, uv.x) * smoothstep(1.0, 0.9, uv.x);
}
)";

inline const char *szZoomMouse = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

mat3 rotX(float a){float s=sin(a),c=cos(a);return mat3(1,0,0, 0,c,-s, 0,s,c);}
mat3 rotY(float a){float s=sin(a),c=cos(a);return mat3(c,0,s, 0,1,0, -s,0,c);}
mat3 rotZ(float a){float s=sin(a),c=cos(a);return mat3(c,-s,0, s,c,0, 0,0,1);}

void main(void){
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float aspect=iResolution.x/iResolution.y;
    vec2 ar=vec2(aspect,1.0);
    vec2 m=(iMouse.z>0.5)?(iMouse.xy/iResolution):vec2(0.5);
    vec2 p=(tc-m)*ar;
    vec3 v=vec3(p,1.0);
    float ax=0.25*sin(time*0.7)*iAmplitude;
    float ay=0.25*cos(time*0.6)*iAmplitude;
    float az=0.4*time;
    mat3 R=rotZ(az)*rotY(ay)*rotX(ax);
    vec3 r=R*v;
    float persp=0.6;
    float zf=1.0/(1.0+r.z*persp);
    vec2 q=r.xy*zf;
    float eps=1e-6;
    float base=1.72;
    float period=log(base);
    float t=time*0.5;
    float rad=length(q)+eps;
    float ang=atan(q.y,q.x)+t*0.3;
    float k=fract((log(rad)-t)/period);
    float rw=exp(k*period);
    vec2 qwrap=vec2(cos(ang),sin(ang))*rw;
    float N=8.0;
    float stepA=6.28318530718/N;
    float a=atan(qwrap.y,qwrap.x)+time*0.05;
    a=mod(a,stepA);
    a=abs(a-stepA*0.5);
    vec2 kdir=vec2(cos(a),sin(a));
    vec2 kaleido=kdir*length(qwrap);
    vec2 uv=kaleido/ar+m;
    uv=fract(uv);
    vec4 texColor = texture(textTexture,uv);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
}
)";

inline const char *szDrain = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float loopDuration = 25.0;
    float currentTime = mod(time, loopDuration);
    vec2 normCoord = (tc * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
    normCoord.x = abs(normCoord.x);
    float dist = length(normCoord);
    float angle = atan(normCoord.y, normCoord.x);
    float spiralSpeed = 5.0 * iFrequency;
    float inwardSpeed = currentTime / loopDuration;
    angle += (1.0 - smoothstep(0.0, 8.0, dist)) * currentTime * spiralSpeed * iAmplitude;
    dist *= 1.0 - inwardSpeed;
    vec2 spiralCoord = vec2(cos(angle), sin(angle)) * tan(dist);
    spiralCoord = (spiralCoord / vec2(iResolution.x / iResolution.y, 1.0) + 1.0) / 2.0;
    vec4 texColor = texture(textTexture, spiralCoord);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *szUfo = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

mat3 rotX(float a){float s=sin(a),c=cos(a);return mat3(1,0,0, 0,c,-s, 0,s,c);}
mat3 rotY(float a){float s=sin(a),c=cos(a);return mat3(c,0,s, 0,1,0, -s,0,c);}
mat3 rotZ(float a){float s=sin(a),c=cos(a);return mat3(c,-s,0, s,c,0, 0,0,1);}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);

    vec2 p2 = (tc - m) * ar;
    float ax = 0.25 * sin(time * 0.7) * iAmplitude;
    float ay = 0.25 * cos(time * 0.6) * iAmplitude;
    float az = time * 0.5;

    vec3 p3 = vec3(p2, 1.0);
    mat3 R = rotZ(az) * rotY(ay) * rotX(ax);
    vec3 r = R * p3;

    float k = 0.6;
    float zf = 1.0 / (1.0 + r.z * k);
    vec2 q = r.xy * zf;

    float dist = length(p2);
    float scale = 1.0 + 0.2 * iAmplitude * sin(dist * 15.0 * iFrequency - time * 2.0);
    q *= scale;

    vec2 uv = q / ar + m;
    uv = clamp(uv, 0.0, 1.0);
    vec4 texColor = texture(textTexture, uv);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *szWave = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform float time_f;
uniform sampler2D textTexture;
uniform vec2 iResolution;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 normCoord = gl_FragCoord.xy / iResolution.xy;
    float diagonalDistance = (normCoord.x + normCoord.y - 1.0) * sqrt(2.0);
    float antiDiagonalDistance = (normCoord.x - normCoord.y) * sqrt(2.0);
    float diagonalWave = sin((diagonalDistance + time) * 5.0 * iFrequency);
    float antiDiagonalWave = cos((antiDiagonalDistance + time) * 5.0 * iFrequency);
    float combinedWave = (diagonalWave + antiDiagonalWave) * 0.5;
    float waveAmt = 0.202 * iAmplitude;
    vec2 waveAdjusted = vec2(tc.x + combinedWave * waveAmt, tc.y + combinedWave * waveAmt);
    vec4 texColor = texture(textTexture, waveAdjusted);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *szUfoWarp = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

mat3 rotX(float a){float s=sin(a),c=cos(a);return mat3(1,0,0, 0,c,-s, 0,s,c);}
mat3 rotY(float a){float s=sin(a),c=cos(a);return mat3(c,0,s, 0,1,0, -s,0,c);}
mat3 rotZ(float a){float s=sin(a),c=cos(a);return mat3(c,-s,0, s,c,0, 0,0,1);}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = 1.0 - abs(1.0 - 2.0 * tc);
    uv = uv - floor(uv);     
    vec2 p2 = (uv - m) * ar;
    float ax = 0.25 * sin(time * 0.7) * iAmplitude;
    float ay = 0.25 * cos(time * 0.6) * iAmplitude;
    float az = time * 0.5;
    vec3 p3 = vec3(p2, 1.0);
    mat3 R = rotZ(az) * rotY(ay) * rotX(ax);
    vec3 r = R * p3;
    float k = 0.6;
    float zf = 1.0 / (1.0 + r.z * k);
    vec2 q = r.xy * zf;
    float dist = length(p2);
    float scale = 1.0 + 0.2 * iAmplitude * sin(dist * 15.0 * iFrequency - time * 2.0);
    q *= scale;
    vec2 uvx = q / ar + m;
    uv = clamp(uvx, 0.0, 1.0);
    vec4 texColor = texture(textTexture, uvx);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *szByMouse = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

mat3 rotX(float a){float s=sin(a),c=cos(a);return mat3(1,0,0, 0,c,-s, 0,s,c);}
mat3 rotY(float a){float s=sin(a),c=cos(a);return mat3(c,0,s, 0,1,0, -s,0,c);}
mat3 rotZ(float a){float s=sin(a),c=cos(a);return mat3(c,-s,0, s,c,0, 0,0,1);}

vec4 color_main(float time) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = 1.0 - abs(1.0 - 2.0 * tc);
    uv = uv - floor(uv);     
    vec2 p2 = (uv - m) * ar;
    float ax = 0.25 * sin(time * 0.7) * iAmplitude;
    float ay = 0.25 * cos(time * 0.6) * iAmplitude;
    float az = time * 0.5;
    vec3 p3 = vec3(p2, 1.0);
    mat3 R = rotZ(az) * rotY(ay) * rotX(ax);
    vec3 r = R * p3;
    float k = 0.6;
    float zf = 1.0 / (1.0 + r.z * k);
    vec2 q = r.xy * zf;
    float dist = length(p2);
    float scale = 1.0 + 0.2 * iAmplitude * sin(dist * 15.0 * iFrequency - time * 2.0);
    q *= scale;
    vec2 uvx = q / ar + m;
    uv = clamp(uvx, 0.0, 1.0);
    return texture(textTexture, uvx);
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 uv = tc;
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 d = uv - m;
    float dist = length(d);
    float r = 0.35;
    float w = 1.0 - smoothstep(0.0, r, dist);
    vec2 warp = normalize(d + 1e-5) * sin(dist * 24.0 * iFrequency - time * 4.0) * 0.25 * iAmplitude * w;
    vec2 warpedCoords = uv + warp;
    warpedCoords.x = pingPong(warpedCoords.x + time * 0.05, 1.0);
    warpedCoords.y = pingPong(warpedCoords.y + time * 0.05, 1.0);
    vec4 texColor = mix(texture(textTexture, warpedCoords), color_main(time), 0.5);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *szDeform = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 normCoord = tc;
    float warpAmount = sin(time) * iAmplitude;
    vec2 warp = vec2(
        sin(normCoord.y * 10.0 * iFrequency + time) * warpAmount,
        cos(normCoord.x * 10.0 * iFrequency + time) * warpAmount
    );
    vec2 warpedCoord = normCoord + warp;
    vec4 texColor = texture(textTexture, warpedCoord);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *szGeo = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 uv = tc * iResolution / vec2(iResolution.y);
    float time_t = pingPong(time, 15.0);
    float angle = time;
    vec2 center = vec2(0.5, 0.5) * iResolution / vec2(iResolution.y);
    vec2 toCenter = uv - center;
    float radius = length(toCenter);
    float theta = atan(toCenter.y, toCenter.x) + time;
    float pattern = abs(sin(12.0 * iFrequency * theta) * cos(12.0 * iFrequency * radius));
    vec3 colorShift = vec3(0.5 * sin(time) + 0.5, 0.5 * cos(time) + 0.5, sin(radius - time));
    vec3 finalColor = (0.2 + 0.8 * pattern) + colorShift * pattern * iAmplitude;
    vec4 text_color = texture(textTexture, tc);
    vec4 fc = vec4(finalColor, 1.0);
    color = sin(mix(text_color, fc, 0.3) * length(time_t));
    color.rgb = applyColorAdjustments(color.rgb);
    color.a = 1.0;
})";

inline const char *szSmooth = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

vec4 xor_RGB(vec4 icolor, vec4 source) {
    ivec3 int_color = ivec3(icolor.rgb * 255.0);
    ivec3 isource = ivec3(source.rgb * 255.0);
    for(int i = 0; i < 3; ++i) {
        int_color[i] = int_color[i] ^ isource[i];
        int_color[i] = int_color[i] % 256;
        icolor[i] = float(int_color[i]) / 255.0;
    }
    icolor.a = 1.0;
    return icolor;
}

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec4 enhancedBlur(sampler2D image, vec2 uv, vec2 resolution) {
    vec2 texelSize = 1.0 / resolution;
    vec4 result = vec4(0.0);
    float totalWeight = 0.0;
    int radius = int(3.0 + iQuality * 2.0);
    for (int x = -5; x <= 5; ++x) {
        for (int y = -5; y <= 5; ++y) {
            if (abs(x) > radius || abs(y) > radius) continue;
            float distance = length(vec2(float(x), float(y)));
            float weight = exp(-distance * distance / (2.0 * float(radius) * float(radius)));
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(image, uv + offset) * weight;
            totalWeight += weight;
        }
    }
    return result / totalWeight;
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 uv = tc;
    uv.x += sin(uv.y * 20.0 * iFrequency + time * 2.0) * 0.005 * iAmplitude;
    uv.y += cos(uv.x * 20.0 * iFrequency + time * 2.0) * 0.005 * iAmplitude;
    vec4 tcolor = enhancedBlur(textTexture, uv, iResolution);
    float time_t = pingPong(time * 0.3, 5.0) + 1.0;
    vec4 modColor = vec4(
        tcolor.r * (0.5 + 0.5 * sin(time + uv.x * 10.0 * iFrequency)),
        tcolor.g * (0.5 + 0.5 * sin(time + uv.y * 10.0 * iFrequency)),
        tcolor.b * (0.5 + 0.5 * sin(time + (uv.x + uv.y) * 5.0 * iFrequency)),
        tcolor.a
    );
    color = xor_RGB(sin(tcolor * time_t), modColor);
    color.rgb = applyColorAdjustments(color.rgb);
    color.rgb = pow(color.rgb, vec3(1.2));
})";

inline const char *szHue = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

const int SEGMENTS = 6;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec4 adjustHue(vec4 c, float a) {
    float U = cos(a);
    float W = sin(a);
    mat3 R = mat3(
        0.299,  0.587,  0.114,
        0.299,  0.587,  0.114,
        0.299,  0.587,  0.114
    ) + mat3(
        0.701, -0.587, -0.114,
       -0.299,  0.413, -0.114,
       -0.300, -0.588,  0.886
    ) * U
      + mat3(
         0.168,  0.330, -0.497,
        -0.328,  0.035,  0.292,
         1.250, -1.050, -0.203
    ) * W;
    return vec4(R * c.rgb, c.a);
}

void main() {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 m = (iMouse.z > 0.5 || iMouse.w > 0.5) ? iMouse.xy / iResolution : vec2(0.5);
    vec2 uvn = (tc - m) * iResolution / min(iResolution.x, iResolution.y);
    float r = length(uvn);
    float ang = atan(uvn.y, uvn.x);
    float seg = 6.28318530718 / float(SEGMENTS);
    float swirlBase = 2.5 * iAmplitude;
    float swirlTime = time * 0.5;
    float swirlMouse = mix(0.0, 3.0, smoothstep(0.0, 0.6, r));
    ang += (swirlBase + swirlMouse) * sin(swirlTime + r * 4.0 * iFrequency);
    ang = mod(ang, seg);
    ang = abs(ang - seg * 0.5);
    vec2 kUV = vec2(cos(ang), sin(ang)) * r;
    float rip = sin(r * 12.0 * iFrequency - pingPong(time, 10.0) * 10.0) * exp(-r * 4.0);
    kUV += rip * 0.01 * iAmplitude;
    vec2 scale = vec2(1.0) / (iResolution / min(iResolution.x, iResolution.y));
    vec2 st = kUV * scale + m;
    float off = 0.003 * sin(time * 0.5) * iAmplitude;
    vec4 c = texture(textTexture, st);
    c += texture(textTexture, st + vec2(off, 0.0));
    c += texture(textTexture, st + vec2(-off, off));
    c += texture(textTexture, st + vec2(off * 0.5, -off));
    c *= 0.25;
    float hueShift = time * 2.0 + rip * 2.0;
    vec4 hc = adjustHue(c, hueShift);
    vec3 rgb = hc.rgb;
    float avg = (rgb.r + rgb.g + rgb.b) / 3.0;
    rgb = mix(vec3(avg), rgb, 1.5);
    rgb *= 1.1;
    vec4 outc = vec4(rgb, 1.0);
    outc = mix(clamp(outc, 0.0, 1.0), texture(textTexture, tc), 0.5);
    outc = sin(outc * pingPong(time, 10.0) + 2.0);
    outc.rgb = applyColorAdjustments(outc.rgb);
    outc.a = 1.0;
    color = outc;
})";

inline const char *szkMouse = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

mat3 rotX(float a){float s=sin(a),c=cos(a);return mat3(1,0,0, 0,c,-s, 0,s,c);}
mat3 rotY(float a){float s=sin(a),c=cos(a);return mat3(c,0,s, 0,1,0, -s,0,c);}
mat3 rotZ(float a){float s=sin(a),c=cos(a);return mat3(c,-s,0, s,c,0, 0,0,1);}

void main(void){
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float aspect=iResolution.x/iResolution.y;
    vec2 ar=vec2(aspect,1.0);
    vec2 m=(iMouse.z>0.5)?(iMouse.xy/iResolution):vec2(0.5);

    vec2 p=(tc-m)*ar;
    vec3 v=vec3(p,1.0);
    float ax=0.25*sin(time*0.7)*iAmplitude;
    float ay=0.25*cos(time*0.6)*iAmplitude;
    float az=0.4*time;
    mat3 R=rotZ(az)*rotY(ay)*rotX(ax);
    vec3 r=R*v;
    float persp=0.6;
    float zf=1.0/(1.0+r.z*persp);
    vec2 q=r.xy*zf;

    float eps=1e-6;
    float base=1.72;
    float period=log(base);
    float t=time*0.5;
    float rad=length(q)+eps;
    float ang=atan(q.y,q.x)+t*0.3;
    float k=fract((log(rad)-t)/period);
    float rw=exp(k*period);
    vec2 qwrap=vec2(cos(ang),sin(ang))*rw;

    float N=8.0;
    float stepA=6.28318530718/N;
    float a=atan(qwrap.y,qwrap.x)+time*0.05;
    a=mod(a,stepA);
    a=abs(a-stepA*0.5);
    vec2 kdir=vec2(cos(a),sin(a));
    vec2 kaleido=kdir*length(qwrap);

    vec2 uv=kaleido/ar+m;
    uv=fract(uv);
    vec4 texColor = texture(textTexture,uv);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *szDrum = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 d = tc - m;
    float r = length(d);
    vec2 dir = d / max(r, 1e-6);
    float waveLength = 0.05 / iFrequency;
    float amplitude = 0.02 * iAmplitude;
    float speed = 2.0 * iSpeed;
    float ripple = sin((r / waveLength - time * speed) * 6.2831853);
    vec2 uv = tc + dir * ripple * amplitude;
    vec4 texColor = texture(textTexture, uv);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *szColorSwirl = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform float alpha;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 m = (iMouse.z > 0.5 ? iMouse.xy : 0.5 * iResolution) / iResolution;
    vec2 uv = 1.0 - abs(1.0 - 2.0 * tc);
    uv = uv - floor(uv);  
    uv = (uv - m) * vec2(iResolution.x / iResolution.y, 1.0);
    float t = time * 0.5;
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);
    angle += t;
    float radMod = pingPong(radius + t * 0.5, 0.5);
    float wave = sin(radius * 10.0 * iFrequency - t * 5.0) * 0.5 + 0.5;
    float r = sin(angle * 3.0 + radMod * 10.0 * iAmplitude + wave * 6.2831);
    float g = sin(angle * 4.0 - radMod * 8.0 * iAmplitude + wave * 4.1230);
    float b = sin(angle * 5.0 + radMod * 12.0 * iAmplitude - wave * 3.4560);
    vec3 col = vec3(r, g, b) * 0.5 + 0.5;
    vec3 texColor = texture(textTexture, tc).rgb;
    col = mix(col, texColor, 0.3);
    col = sin(col * pingPong(time, 8.0) + 2.0);
    col = applyColorAdjustments(col);
    color = vec4(col, alpha);
})";

inline const char *szMouseZoom = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

mat3 rotX(float a){float s=sin(a),c=cos(a);return mat3(1,0,0, 0,c,-s, 0,s,c);}
mat3 rotY(float a){float s=sin(a),c=cos(a);return mat3(c,0,s, 0,1,0, -s,0,c);}
mat3 rotZ(float a){float s=sin(a),c=cos(a);return mat3(c,-s,0, s,c,0, 0,0,1);}

void main(void){
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float aspect=iResolution.x/iResolution.y;
    vec2 ar=vec2(aspect,1.0);
    vec2 m=(iMouse.z>0.5)?(iMouse.xy/iResolution):vec2(0.5);
    vec2 cv = 1.0 - abs(1.0 - 2.0 * tc);
    cv = cv - floor(cv);     
    vec2 p=(cv-m)*ar;
    vec3 v=vec3(p,1.0);
    float ax=0.25*sin(time*0.7)*iAmplitude;
    float ay=0.25*cos(time*0.6)*iAmplitude;
    float az=0.4*time;
    mat3 R=rotZ(az)*rotY(ay)*rotX(ax);
    vec3 r=R*v;
    float persp=0.6;
    float zf=1.0/(1.0+r.z*persp);
    vec2 q=r.xy*zf;
    float eps=1e-6;
    float base=1.72;
    float period=log(base);
    float t=time*0.5;
    float rad=length(q)+eps;
    float ang=atan(q.y,q.x)+t*0.3;
    float k=fract((log(rad)-t)/period);
    float rw=exp(k*period);
    vec2 qwrap=vec2(cos(ang),sin(ang))*rw;
    float N=8.0;
    float stepA=6.28318530718/N;
    float a=atan(qwrap.y,qwrap.x)+time*0.05;
    a=mod(a,stepA);
    a=abs(a-stepA*0.5);
    vec2 kdir=vec2(cos(a),sin(a));
    vec2 kaleido=kdir*length(qwrap);
    vec2 uv=kaleido/ar+m;
    uv=fract(uv);
    vec4 texColor = texture(textTexture,uv);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *szRev2 = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
out vec4 color;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

void main() {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 uv = 1.0 - abs(1.0 - 2.0 * tc);
    uv = uv - floor(uv);  
    vec2 centeredUV = uv * 2.0 - 1.0;
    float angle = atan(centeredUV.y, centeredUV.x);
    float radius = length(centeredUV);
    float spin = time * 0.5;
    angle += floor(mod(angle + 3.14159, 3.14159 / 4.0)) * spin;
    angle = angle * pingPong(sin(time), 30.0) * iAmplitude;
    vec2 rotatedUV = vec2(cos(angle), sin(angle)) * radius;
    rotatedUV = abs(mod(rotatedUV, 2.0) - 1.0);
    vec4 texColor = texture(textTexture, rotatedUV * 0.5 + 0.5);
    vec3 gradientEffect = vec3(
        0.5 + 0.5 * sin(time + uv.x * 15.0 * iFrequency),
        0.5 + 0.5 * cos(time + uv.y * 15.0 * iFrequency),
        0.5 + 0.5 * sin(time + (uv.x + uv.y) * 15.0 * iFrequency)
    );
    vec3 finalColor = mix(texColor.rgb, gradientEffect, 0.3);
    finalColor = applyColorAdjustments(finalColor);
    color = vec4(finalColor, texColor.a);
})";

inline const char *szFish = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : (length * 2.0 - m);
}
float h1(float n){ return fract(sin(n)*43758.5453123); }
vec2 h2(float n){ return fract(sin(vec2(n,n+1.0))*vec2(43758.5453,22578.1459)); }

void main(void){
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float time_t = pingPong(time, 10.0) + 2.0;
    vec2 m = iMouse.xy / iResolution;
    bool hasMouse = iMouse.z > 0.5 || iMouse.w > 0.5;
    float rate = 0.8;
    float t = time * rate;
    float t0 = floor(t);
    float a = fract(t);
    float w = a*a*(3.0-2.0*a);
    vec2 p0 = vec2(0.15) + h2(t0) * 0.7;
    vec2 p1 = vec2(0.15) + h2(t0 + 1.0) * 0.7;
    vec2 autoCenter = mix(p0, p1, w);
    vec2 center = hasMouse ? m : autoCenter;
    vec2 uv = 1.0 - abs(1.0 - 2.0 * tc);
    uv = uv - floor(uv);    
    uv = uv - center;
    float lenv = length(uv);
    float factor = sqrt(lenv) * 0.5 * iAmplitude;
    float s = 1.0 + sin(factor * time_t * iFrequency);
    uv *= s;
    vec4 texColor = texture(textTexture, uv + center);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *szRipplePrism = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
uniform sampler2D textTexture;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x,float length){
    float m=mod(x,length*2.0);
    return m<=length?m:length*2.0-m;
}

void main(void){
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float A=clamp(amp * iAmplitude,0.0,1.0);
    float U=clamp(uamp * iFrequency,0.0,1.0);
    float time_z=pingPong(time,4.0)+0.5;

    vec2 uv=tc;
    vec2 center=(iMouse.z>0.5||iMouse.w>0.5)?(iMouse.xy/iResolution):vec2(0.5);
    vec2 normPos=(uv-center)*vec2(iResolution.x/iResolution.y,1.0);

    float dist=length(normPos);
    float phase=sin(dist*10.0*iFrequency-time*4.0);
    float phaseGain=mix(0.6,1.4,A);
    vec2 tcAdjusted=uv+(normPos*0.305*phase*phaseGain*iAmplitude);

    float dispersionScale=0.02*(0.5+U);
    vec2 dispersionOffset=normPos*dist*dispersionScale;

    vec2 tcAdjustedR=tcAdjusted-dispersionOffset;
    vec2 tcAdjustedG=tcAdjusted;
    vec2 tcAdjustedB=tcAdjusted+dispersionOffset;

    float r=texture(textTexture,tcAdjustedR).r;
    float g=texture(textTexture,tcAdjustedG).g;
    float b=texture(textTexture,tcAdjustedB).b;

    vec3 col = applyColorAdjustments(vec3(r,g,b));
    color=vec4(col,1.0);
})";

inline const char *szMirrorMouse = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec2 rotateUV(vec2 uv, float angle, vec2 center) {
    float s = sin(angle);
    float c = cos(angle);
    uv -= center;
    uv = mat2(c, -s, s, c) * uv;
    uv += center;
    return uv;
}

vec2 reflectUV(vec2 uv, float segments, vec2 center) {
    float angle = atan(uv.y - center.y, uv.x - center.x);
    float radius = length(uv - center);
    float segmentAngle = 2.0 * 3.14159265359 / segments;
    angle = mod(angle, segmentAngle);
    angle = abs(angle - segmentAngle * 0.5);
    return vec2(cos(angle), sin(angle)) * radius + center;
}

vec2 fractalZoom(vec2 uv, float zoom, float time, vec2 center) {
    int iterations = int(3.0 + iQuality * 2.0);
    for (int i = 0; i < 5; i++) {
        if (i >= iterations) break;
        uv = abs((uv - center) * zoom) - 0.5 + center;
        uv = rotateUV(uv, sin(time * pingPong(time, 10.0)) * 0.1 * iAmplitude, center);
    }
    return uv;
}

void main() {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 uv = 1.0 - abs(1.0 - 2.0 * tc);
    uv = uv - floor(uv);   
    uv = uv * iResolution / vec2(iResolution.y);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 center = m * iResolution / vec2(iResolution.y);
    vec4 originalTexture = texture(textTexture, tc);
    vec2 kaleidoUV = reflectUV(uv, 6.0, center);
    float zoom = 1.5 + 0.5 * sin(time * 0.5) * iAmplitude;
    kaleidoUV = fractalZoom(kaleidoUV, zoom, time, center);
    kaleidoUV = rotateUV(kaleidoUV, time * 0.2, center);
    vec4 kaleidoColor = texture(textTexture, kaleidoUV);
    float blendFactor = 0.6;
    vec4 blendedColor = mix(kaleidoColor, originalTexture, blendFactor);
    blendedColor.rgb *= 0.5 + 0.5 * sin(kaleidoUV.xyx * iFrequency + time);
    color = sin(blendedColor * pingPong(time, 10.0));
    vec4 t = texture(textTexture, tc);
    color = color * t * 0.8;
    color = sin(color * pingPong(time, 15.0));
    color.rgb = applyColorAdjustments(color.rgb);
    color.a = 1.0;
})";

inline const char *szFracMouse = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length){
    float m = mod(x, length*2.0);
    return m <= length ? m : length*2.0 - m;
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    int iterations = int(3.0 + iQuality * 3.0);
    for(int i=0;i<6;i++){
        if (i >= iterations) break;
        p = abs((p - c) * (zoom + 0.15*iAmplitude*sin(t*0.35*iFrequency+float(i)))) - 0.5 + c;
        p = rotateUV(p, t*0.12 + float(i)*0.07, c, aspect);
    }
    return p;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main(){
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = 1.0 - abs(1.0 - 2.0 * tc);
    uv = uv - floor(uv);     
    vec4 originalTexture = texture(textTexture, tc);
    float seg = 6.0 + 2.0*sin(time*0.33*iFrequency);
    vec2 kUV = reflectUV(uv, seg, m, aspect);
    float foldZoom = 1.45 + 0.55 * iAmplitude * sin(time * 0.42);
    kUV = fractalFold(kUV, foldZoom, time, m, aspect);
    kUV = rotateUV(kUV, time * 0.23, m, aspect);
    vec2 p = (kUV - m) * ar;
    float base = 1.82 + 0.18*sin(time*0.2);
    float period = log(base);
    float tz = time * 0.65;
    float r = length(p) + 1e-6;
    float ang = atan(p.y, p.x) + tz * 0.35 + 0.35*iAmplitude*sin(r*9.0*iFrequency + time*0.8);
    float k = fract((log(r) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap*1.045) / ar + m);
    vec2 u2 = fract((pwrap*0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time*1.3)) * vec2(1.0, 1.0/aspect);
    float hue = fract(ang*0.15 + time*0.08 + k*0.5);
    float sat = 0.8 - 0.2*cos(time*0.7 + r*6.0);
    float val = 0.8 + 0.2*sin(time*0.9 + k*6.28318530718);
    vec3 tint = hsv2rgb(vec3(hue, sat, val));
    float ring = smoothstep(0.0, 0.7, sin(log(r+1e-3)*8.0*iFrequency + time*1.2));
    float pulse = 0.5 + 0.5*sin(time*2.0 + r*18.0*iFrequency + k*12.0);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((tc - m)*ar));
    vign = mix(0.85, 1.2, vign);
    float blendFactor = 0.58;
    float rC = texture(textTexture, u0 + off).r;
    float gC = texture(textTexture, u1).g;
    float bC = texture(textTexture, u2 - off).b;
    vec3 kaleidoRGB = vec3(rC, gC, bC);
    vec4 kaleidoColor = vec4(kaleidoRGB, 1.0) * vec4(tint, 1.0);
    vec4 merged = mix(kaleidoColor, originalTexture, blendFactor);
    merged.rgb *= (0.75 + 0.25*ring) * (0.85 + 0.15*pulse) * vign;
    vec3 bloom = merged.rgb * merged.rgb * 0.18 + pow(max(merged.rgb-0.6,0.0), vec3(2.0))*0.12;
    vec3 outCol = merged.rgb + bloom;
    float wob = 0.9 + 0.1*sin(time + r*12.0*iFrequency + k*9.0);
    outCol *= wob;
    vec4 t = texture(textTexture, tc);
    outCol = mix(outCol, outCol*t.rgb, 0.8);
    outCol = sin(outCol * (0.5 + 0.5*pingPong(time, 12.0)));
    outCol = applyColorAdjustments(outCol);
    outCol = clamp(outCol, vec3(0.08), vec3(0.96));
    color = vec4(outCol, 1.0);
})";

inline const char *szCats = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform float time_f;
uniform sampler2D textTexture;
uniform vec2 iResolution;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

vec4 xor_RGB(vec4 icolor, vec4 isourcex) {
    ivec4 isource = ivec4(isourcex.rgba * 255.0);
    ivec3 int_color;
    for(int i = 0; i < 3; ++i) {
        int_color[i] = int(255.0 * icolor[i]);
        int_color[i] ^= isource[i];
        if(int_color[i] > 255)
            int_color[i] %= 255;
        icolor[i] = float(int_color[i]) / 255.0;
    }
    icolor.a = 1.0;
    return icolor;
}

float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

vec2 random2(vec2 st) {
    st = vec2(dot(st, vec2(127.1, 311.7)), dot(st, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(st) * 43758.5453123);
}

vec2 smoothRandom2(float t) {
    float t0 = floor(t);
    float t1 = t0 + 1.0;
    vec2 rand0 = random2(vec2(t0));
    vec2 rand1 = random2(vec2(t1));
    float mix_factor = fract(t);
    return mix(rand0, rand1, smoothstep(0.0, 1.0, mix_factor));
}

vec3 rainbow(float t) {
    t = fract(t);
    float r = abs(t * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(t * 6.0 - 2.0);
    float b = 2.0 - abs(t * 6.0 - 4.0);
    return clamp(vec3(r, g, b), 0.0, 1.0);
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 normPos = (gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0;
    float dist = length(normPos);
    float phase = sin(dist * 10.0 * iFrequency - time * 4.0);
    vec2 tcAdjusted = tc + (normPos * 0.305 * phase * iAmplitude);
    vec4 texColor = texture(textTexture, tcAdjusted);
    vec2 uv = (tc - 0.5) * 2.0;
    uv.x *= iResolution.x / iResolution.y;
    float sine1 = sin(uv.x * 10.0 * iFrequency + time) * cos(uv.y * 10.0 * iFrequency + time);
    float sine2 = sin(uv.y * 15.0 * iFrequency - time) * cos(uv.x * 15.0 * iFrequency - time);
    float sine3 = sin(sqrt(uv.x * uv.x + uv.y * uv.y) * 20.0 * iFrequency + time);
    float pattern = sine1 + sine2 + sine3;
    float colorIntensity = pattern * 0.5 + 0.5;
    vec3 psychedelicColor = vec3(
        sin(colorIntensity * 6.28318 + 0.0) * 0.5 + 0.5,
        sin(colorIntensity * 6.28318 + 2.09439) * 0.5 + 0.5,
        sin(colorIntensity * 6.28318 + 4.18879) * 0.5 + 0.5
    );
    vec4 finalColor = vec4(psychedelicColor, 1.0);
    vec4 xorColor = xor_RGB(finalColor, texColor);
    vec2 uv2 = tc * 2.0 - 1.0;
    uv2.y *= iResolution.y / iResolution.x;
    float wave = sin(uv2.x * 10.0 * iFrequency + time * 2.0) * 0.1 * iAmplitude;
    vec2 random_direction = smoothRandom2(time) * 0.5;
    float expand = 0.5 + 0.5 * sin(time * 2.0);
    vec2 spiral_uv = uv2 * expand + random_direction;
    float angle = atan(spiral_uv.y + wave, spiral_uv.x) + time * 2.0;
    vec3 rainbow_color = rainbow(angle / (2.0 * 3.14159));
    vec4 original_color = texture(textTexture, tc);
    vec3 blended_color = mix(original_color.rgb, rainbow_color, 0.5);
    float time_t = mod(time, 30.0);
    vec4 finalBlendedColor = vec4(sin(blended_color * time_t), original_color.a);   
    color = mix(xorColor, finalBlendedColor, 0.5);
    color.rgb = applyColorAdjustments(color.rgb);
})";

inline const char *szColorMouse = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform float alpha;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = (tc - m) * vec2(iResolution.x / iResolution.y, 1.0);
    float t = time * 0.7;
    float beat = abs(sin(time * 3.14159)) * 0.2 + 0.8;
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);
    angle += t * 0.5;
    float radMod = pingPong(radius + t * 0.3, 0.5);
    float wave = sin(radius * 10.0 * iFrequency - t * 6.0) * 0.5 + 0.5;
    float distortion = sin((radius + t * 0.5) * 8.0 * iFrequency) * beat * 0.1 * iAmplitude;
    vec2 dir = vec2(cos(angle), sin(angle));
    float r = sin(angle * 3.0 + radMod * 10.0 * iAmplitude + wave * 6.2831);
    float g = sin(angle * 4.0 - radMod * 8.0 * iAmplitude + wave * 4.1230);
    float b = sin(angle * 5.0 + radMod * 12.0 * iAmplitude - wave * 3.4560);
    vec3 col = vec3(r, g, b) * 0.5 + 0.5;
    vec3 texColor = texture(textTexture, tc).rgb;
    col = mix(col, texColor, 0.3);
    col = sin(col * pingPong(time, 8.0) + 2.0);
    col = applyColorAdjustments(col);
    color = vec4(col, alpha);
})";

inline const char *szTwistFull = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = tc - m;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    
    float dist = length(uv);
    float angle = atan(uv.y, uv.x);
    
    // Twist effect based on distance
    float twistAmount = 3.0 * iAmplitude * sin(time * 0.5);
    angle += twistAmount * (1.0 - dist) * sin(dist * 10.0 * iFrequency - time * 2.0);
    
    // Ripple effect
    float ripple = sin(dist * 20.0 * iFrequency - time * 4.0) * 0.02 * iAmplitude;
    dist += ripple;
    
    // Reconstruct UV
    vec2 newUV;
    newUV.x = cos(angle) * dist / aspect;
    newUV.y = sin(angle) * dist;
    newUV += m;
    
    // Chromatic aberration
    float chromaOffset = 0.005 * iAmplitude;
    float r = texture(textTexture, newUV + vec2(chromaOffset, 0.0)).r;
    float g = texture(textTexture, newUV).g;
    float b = texture(textTexture, newUV - vec2(chromaOffset, 0.0)).b;
    
    vec3 col = applyColorAdjustments(vec3(r, g, b));
    color = vec4(col, 1.0);
})";

inline const char *szBowlByTime = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = tc - m;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    
    float dist = length(uv);
    
    // Bowl distortion - bulge in the center
    float bowlStrength = 0.5 * iAmplitude * sin(time * 0.3);
    float bowlEffect = 1.0 + bowlStrength * (1.0 - dist * 2.0);
    bowlEffect = max(bowlEffect, 0.5);
    
    // Time-based wave
    float wave = sin(dist * 15.0 * iFrequency - time * 3.0) * 0.03 * iAmplitude;
    
    vec2 newUV = uv * bowlEffect + wave * normalize(uv + 0.001);
    newUV.x /= aspect;
    newUV += m;
    
    // Wrap coordinates
    newUV.x = pingPong(newUV.x, 1.0);
    newUV.y = pingPong(newUV.y, 1.0);
    
    vec4 texColor = texture(textTexture, newUV);
    
    // Add subtle color shift based on distortion
    vec3 col = texColor.rgb;
    col.r += sin(time + dist * 5.0) * 0.1 * iAmplitude;
    col.b += cos(time + dist * 5.0) * 0.1 * iAmplitude;
    
    col = applyColorAdjustments(col);
    color = vec4(col, texColor.a);
})";

inline const char *szPebble = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    
    // Create pebble/droplet ripple effect
    vec2 uv = tc;
    float aspect = iResolution.x / iResolution.y;
    
    // Multiple ripple centers
    for(int i = 0; i < 3; i++) {
        float fi = float(i);
        vec2 center = vec2(
            0.3 + 0.4 * sin(time * 0.5 + fi * 2.1),
            0.3 + 0.4 * cos(time * 0.4 + fi * 1.7)
        );
        
        if(iMouse.z > 0.5) {
            center = mix(center, m, 0.5);
        }
        
        vec2 d = (uv - center) * vec2(aspect, 1.0);
        float dist = length(d);
        
        float ripple = sin(dist * 30.0 * iFrequency - time * 5.0 - fi * 1.5) * 0.015 * iAmplitude;
        ripple *= smoothstep(0.5, 0.0, dist);
        
        uv += normalize(d + 0.001) * ripple;
    }
    
    // Wrap coordinates
    uv.x = pingPong(uv.x, 1.0);
    uv.y = pingPong(uv.y, 1.0);
    
    vec4 texColor = texture(textTexture, uv);
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
})";

inline const char *szMirrorPutty = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
)" COMMON_UNIFORMS COLOR_HELPERS R"(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec2 mirror(vec2 uv) {
    return 1.0 - abs(1.0 - 2.0 * fract(uv * 0.5));
}

void main(void) {
    float time = time_f * iSpeed;
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    
    vec2 uv = tc;
    float aspect = iResolution.x / iResolution.y;
    
    // Putty-like stretching effect
    vec2 center = m;
    vec2 d = (uv - center) * vec2(aspect, 1.0);
    float dist = length(d);
    
    // Elastic putty deformation
    float stretch = sin(time * 2.0) * 0.3 * iAmplitude;
    float squash = cos(time * 2.0 + 1.5) * 0.2 * iAmplitude;
    
    vec2 deform = vec2(
        sin(uv.y * 10.0 * iFrequency + time * 3.0) * stretch,
        cos(uv.x * 10.0 * iFrequency + time * 2.5) * squash
    );
    
    // Add wave distortion
    float wave = sin(dist * 15.0 * iFrequency - time * 4.0) * 0.05 * iAmplitude;
    deform += normalize(d + 0.001) * wave * (1.0 - dist);
    
    uv += deform;
    
    // Mirror effect
    uv = mirror(uv);
    
    // Chromatic aberration
    float chromaAmt = 0.008 * iAmplitude * (1.0 + sin(time));
    vec2 chromaDir = normalize(d + 0.001);
    
    float r = texture(textTexture, uv + chromaDir * chromaAmt).r;
    float g = texture(textTexture, uv).g;
    float b = texture(textTexture, uv - chromaDir * chromaAmt).b;
    
    vec3 col = applyColorAdjustments(vec3(r, g, b));
    
    // Add subtle color cycling
    col.r += sin(time * 0.5 + dist * 3.0) * 0.1 * iAmplitude;
    col.g += sin(time * 0.6 + dist * 4.0) * 0.1 * iAmplitude;
    col.b += sin(time * 0.7 + dist * 5.0) * 0.1 * iAmplitude;
    
    col = clamp(col, 0.0, 1.0);
    color = vec4(col, 1.0);
})";

inline const char *src_frac_shader02_dmdi6i_zoom_xor = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

vec4 xor_RGB(vec4 icolor, vec4 source){
    ivec3 int_color;
    ivec4 isource = ivec4(source * 255.0);
    for(int i=0;i<3;++i){
        int_color[i] = int(255.0*icolor[i]);
        int_color[i] = int_color[i] ^ isource[i];
        if(int_color[i]>255) int_color[i] = int_color[i] % 255;
        icolor[i] = float(int_color[i]) / 255.0;
    }
    icolor.a = 1.0;
    return icolor;
}

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    float aspect = iResolution.x / iResolution.y;
    vec2 m = (iMouse.z > 0.5 || iMouse.w > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);

    float zoomPhase = time_f * 0.12;
    float zCycle = floor(zoomPhase);
    float zLocal = fract(zoomPhase);
    float tri = 1.0 - abs(zLocal * 2.0 - 1.0);
    float sgn = mix(-1.0, 1.0, step(0.5, mod(zCycle, 2.0)));
    float depth = 1.0 + zCycle * 0.35;
    float zoomExp = sgn * tri * depth;
    float zoom = pow(1.45, zoomExp);

    vec2 z = TexCoord - m;
    z.x *= aspect;
    z /= zoom;
    z.x /= aspect;
    vec2 zoomTC = fract(z + m);

    vec4 baseTex = texture(textTexture, zoomTC);

    vec2 uv = zoomTC * 2.0 - 1.0;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);

    vec3 baseCol = preBlendColor(zoomTC);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(zoomTC, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((zoomTC - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;

    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalCore = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);

    vec2 center = m;
    vec2 pSwirl = (zoomTC - center) * vec2(1.0, iResolution.y / iResolution.x);
    float intensity = pingPong(time_f, 10.0);
    float angleS = atan(pSwirl.y, pSwirl.x);
    float radiusS = length(pSwirl);
    float swirl = sin(time_f * 0.5) * 0.5 + 0.5;
    angleS += intensity * swirl * sin(radiusS * 10.0 + time_f);
    vec2 qSwirl = vec2(cos(angleS), sin(angleS)) * radiusS;
    vec2 uvSwirl = qSwirl * vec2(1.0, iResolution.x / iResolution.y) + center;

    vec4 texSwirl = texture(textTexture, uvSwirl);
    float fluctuation = sin(time_f * 2.0) * 0.5 + 0.5;
    vec3 xorPalette = mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), fluctuation);
    xorPalette = mix(xorPalette, neonPalette(time_f), 0.5);
    vec4 fluctuatedColor = vec4(xorPalette, 1.0);
    vec4 xorResult = xor_RGB(texSwirl, fluctuatedColor);
    vec3 xorMix = mix(texSwirl.rgb, xorResult.rgb, 0.6);

    float xorStrength = (0.25 + 0.35 * pulse) * vign;
    xorStrength = clamp(xorStrength, 0.0, 1.0);

    vec3 finalRGB = mix(finalCore, xorMix, xorStrength);

    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmdi6i_zoom_xor_amp = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;

const float PI = 3.1415926535897932384626433832795;

float gAmp01;
float gSlow;
float gFast;
float gDetail;

vec4 xor_RGB(vec4 icolor, vec4 source){
    ivec3 int_color;
    ivec4 isource = ivec4(source * 255.0);
    for(int i=0;i<3;++i){
        int_color[i] = int(255.0*icolor[i]);
        int_color[i] = int_color[i] ^ isource[i];
        if(int_color[i]>255) int_color[i] = int_color[i] % 255;
        icolor[i] = float(int_color[i]) / 255.0;
    }
    icolor.a = 1.0;
    return icolor;
}

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    float aAcc = clamp(amp, 0.0, 4.0);
    float aInst = clamp(uamp, 0.0, 4.0);
    float ampMix = clamp(aAcc * 0.6 + aInst * 1.4, 0.0, 4.0);
    gAmp01 = clamp(ampMix / 2.5, 0.0, 1.0);

    gSlow = time_f * mix(0.15, 0.7, gAmp01);
    gFast = time_f * mix(0.6, 3.5, gAmp01);
    gDetail = time_f * mix(0.3, 2.0, gAmp01);

    float aspect = iResolution.x / iResolution.y;
    vec2 m = (iMouse.z > 0.5 || iMouse.w > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);

    float zoomPhase = gSlow * 0.12;
    float zCycle = floor(zoomPhase);
    float zLocal = fract(zoomPhase);
    float tri = 1.0 - abs(zLocal * 2.0 - 1.0);
    float sgn = mix(-1.0, 1.0, step(0.5, mod(zCycle, 2.0)));
    float depth = 1.0 + zCycle * 0.35 + gAmp01 * 1.2;
    float zoomExp = sgn * tri * depth;
    float zoom = pow(1.45, zoomExp);

    vec2 z = TexCoord - m;
    z.x *= aspect;
    z /= zoom;
    z.x /= aspect;
    vec2 zoomTC = fract(z + m);

    vec4 baseTex = texture(textTexture, zoomTC);

    vec2 uv = zoomTC * 2.0 - 1.0;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * gSlow), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);

    vec3 baseCol = preBlendColor(zoomTC);
    float seg = 4.0 + 2.0 * sin(gSlow * 0.33 + gAmp01 * 2.0);
    vec2 kUV = reflectUV(zoomTC, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(gSlow * 0.42 + gAmp01 * 3.0);
    kUV = fractalFold(kUV, foldZoom, gDetail, m, aspect);
    kUV = rotateUV(kUV, gSlow * 0.23 + gAmp01 * 1.2, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(gSlow * 0.2) * (PI * gSlow), 5.0);
    float period = log(base) * pingPong(gSlow * PI, 5.0);
    float tz = gSlow * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + gFast * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(gFast * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((zoomTC - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + gFast * 1.2));
    ring = ring * pingPong(gSlow * PI, 5.0);
    float pulse = 0.5 + 0.5 * sin(gFast * 2.0 + rD * 28.0 + k * 12.0);
    pulse *= mix(0.4, 1.6, gAmp01);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;

    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    outCol *= mix(0.7, 1.5, gAmp01);
    vec3 finalCore = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);

    vec2 center = m;
    vec2 pSwirl = (zoomTC - center) * vec2(1.0, iResolution.y / iResolution.x);
    float intensity = pingPong(gSlow, 10.0) * (0.3 + 1.7 * gAmp01);
    float angleS = atan(pSwirl.y, pSwirl.x);
    float radiusS = length(pSwirl);
    float swirl = sin(gSlow * 0.5) * 0.5 + 0.5;
    angleS += intensity * swirl * sin(radiusS * 10.0 + gFast);
    vec2 qSwirl = vec2(cos(angleS), sin(angleS)) * radiusS;
    vec2 uvSwirl = qSwirl * vec2(1.0, iResolution.x / iResolution.y) + center;

    vec4 texSwirl = texture(textTexture, uvSwirl);
    float fluctuation = sin(gFast * 2.0) * 0.5 + 0.5;
    vec3 xorPalette = mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), fluctuation);
    xorPalette = mix(xorPalette, neonPalette(gFast), 0.5);
    vec4 fluctuatedColor = vec4(xorPalette, 1.0);
    vec4 xorResult = xor_RGB(texSwirl, fluctuatedColor);
    vec3 xorMix = mix(texSwirl.rgb, xorResult.rgb, 0.6);

    float xorStrength = (0.25 + 0.35 * pulse) * vign * mix(0.4, 1.4, gAmp01);
    xorStrength = clamp(xorStrength, 0.0, 1.0);

    vec3 finalRGB = mix(finalCore, xorMix, xorStrength);

    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmdXi = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i = 0; i < 6; i++){
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t){
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.05);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

void main(void){
    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);
    vec2 m = (iMouse.z > 0.5 ? (iMouse.xy / iResolution) : vec2(0.5));

    float t = time_f + iTime;
    float sr = clamp(iSampleRate / 48000.0, 0.25, 4.0);
    float aMix = clamp(amp * 0.7 + uamp * 0.3, 0.0, 20.0);

    float seg = 4.0 + 2.0 * sin(t * 0.25);
    float zoom = 1.45 + 0.45 * sin(t * 0.35 + 0.2 * sr);

    vec2 kUV = reflectUV(TexCoord, seg, m, ar.x);
    kUV = fractalFold(kUV, zoom, t, m, ar.x);
    kUV = rotateUV(kUV, t * 0.22, m, ar.x);

    vec2 dir = (kUV - m) * ar;
    float r = length(dir);
    float ripple = sin(20.0 * r - t * 9.0) * 0.012 / (1.0 + 18.0 * r);
    ripple *= 1.0 / (1.0 + 12.0 * (abs(dFdx(r)) + abs(dFdy(r))));

    vec2 uA = kUV + normalize(dir + 1e-5) * ripple;
    vec2 uB = mix(TexCoord, kUV, 0.88);
    vec2 uC = mix(TexCoord, kUV, 0.94) + vec2(0.002 * sin(t), 0.002 * cos(t));

    vec4 t1 = textureGrad(textTexture, fract(uA), dFdx(uA), dFdy(uA));
    vec4 t2 = textureGrad(textTexture, fract(uB), dFdx(uB), dFdy(uB));
    vec4 t3 = textureGrad(textTexture, fract(uC), dFdx(uC), dFdy(uC));

    float hueBase = fract(t * 0.06);
    float sat = 0.7 + 0.25 * sin(t * 0.4 + sr);
    float val = 0.6 + 0.35 * sin(t * 0.5 + dot(TexCoord, vec2(2.3, 1.9)));
    vec3 tint = hsv2rgb(vec3(hueBase + r * 0.25, sat, val));

    float slowBeat = 0.5 + 0.5 * sin(t * 0.8 + aMix * 0.05);
    float mix1 = 0.5 + 0.5 * sin(t * 0.6);
    float mix2 = 0.5 + 0.5 * cos(t * 0.45);

    vec3 warpCol = mix(t1.rgb, t2.rgb, mix1);
    warpCol = mix(warpCol, t3.rgb, mix2 * 0.5);
    warpCol *= tint * (0.92 + 0.08 * slowBeat);
    vec3 base = textureGrad(textTexture, TexCoord, dFdx(TexCoord), dFdy(TexCoord)).rgb;
    vec3 combined = mix(base, warpCol * 3.0, 0.6); 
    vec3 bloom = combined * combined * 0.18 + pow(max(combined - 0.6, 0.0), vec3(2.0)) * 0.10;
    combined += bloom;

    combined = clamp(combined, vec3(0.0), vec3(1.0));
    color = vec4(combined, 1.0);
}
)SHD";
inline const char *src_frac_shader02_dmdXi_amp = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

const float PI = 3.1415926535897932384626433832795;

float gAmp01;
float gInst01;
float gSlow;
float gFast;
float gDetail;

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i = 0; i < 6; i++){
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t){
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.05);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 limitHighlights(vec3 c){
    float m = max(c.r, max(c.g, c.b));
    if(m > 0.9) c *= 0.9 / m;
    return c;
}

void main(void){
    float aAcc = clamp(amp, 0.0, 20.0);
    float aInst = clamp(uamp, 0.0, 20.0);
    float aMix = clamp(aAcc * 0.7 + aInst * 0.3, 0.0, 20.0);
    gAmp01 = clamp(aMix / 8.0, 0.0, 1.0);
    gInst01 = clamp(aInst / 8.0, 0.0, 1.0);

    float tGlobal = time_f + iTime;
    float fpsNorm = clamp(iFrameRate / 60.0, 0.4, 2.5);
    float t = tGlobal * fpsNorm;

    float sr = clamp(iSampleRate / 48000.0, 0.25, 4.0);
    float chT0 = iChannelTime[0];
    vec2 chRes0 = iChannelResolution[0].xy;
    float chAspect = (chRes0.y > 0.0) ? chRes0.x / chRes0.y : (iResolution.x / iResolution.y);

    float framePhase = float(iFrame) * 0.009 + iTimeDelta * 3.0;
    float frameJitter = sin(framePhase) * 0.004;

    float datePhase = iDate.y * 0.13 + iDate.z * 0.03 + iDate.w * 0.001;

    gSlow   = t * mix(0.18, 0.7, gAmp01);
    gFast   = t * mix(0.7,  4.0, gAmp01);
    gDetail = t * mix(0.35, 2.3, gAmp01);

    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);

    vec2 baseCenter = (iMouse.z > 0.5 ? (iMouse.xy / iResolution) : vec2(0.5));
    vec2 clickCenter = (iMouseClick.x > 0.0 || iMouseClick.y > 0.0)
        ? (iMouseClick / iResolution)
        : baseCenter;
    float clickMix = 0.25 + 0.35 * smoothstep(0.2, 1.0, gAmp01);
    vec2 m = mix(baseCenter, clickCenter, clickMix);

    float aspect = ar.x;

    vec2 uv0 = TexCoord * 2.0 - 1.0;
    uv0.x *= aspect;
    float baseR = length(uv0);

    float segBase = 4.0 + 2.0 * sin(gSlow * 0.25 + datePhase);
    float segAmp = 3.0 * gAmp01 + 4.0 * gInst01;
    float seg = segBase + segAmp * sin(gFast * 0.21 + chT0 * 0.37);

    float zoomBase = 1.35 + 0.35 * sin(gSlow * 0.35 + 0.2 * sr);
    float zoomAmp = 0.5 + 0.9 * gAmp01;
    float zoom = zoomBase + zoomAmp * sin(gFast * 0.27 + baseR * 2.0 + framePhase);

    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = fractalFold(kUV, zoom, gDetail, m, aspect);
    kUV = rotateUV(kUV, gSlow * 0.22 + gInst01 * 1.3, m, aspect + 0.2 * (chAspect - aspect));

    vec2 dir = (kUV - m) * ar;
    float r = length(dir);

    float rippleAmp = mix(0.006, 0.02, gAmp01) * (0.7 + 0.6 * gInst01);
    float rippleFreq = 18.0 + 6.0 * gAmp01 + 8.0 * gInst01;
    float rippleTime = gFast * (0.8 + 0.4 * gInst01 * sr);
    float ripple = sin(rippleFreq * r - rippleTime) * rippleAmp / (1.0 + 18.0 * r);
    ripple *= 1.0 / (1.0 + 12.0 * (abs(dFdx(r)) + abs(dFdy(r))));
    ripple += frameJitter * 0.4;

    vec2 dirN = normalize(dir + 1e-5);
    vec2 uA = kUV + dirN * ripple;
    vec2 uB = mix(TexCoord, kUV, 0.84 + 0.08 * gAmp01);
    vec2 jitter = vec2(0.002 * sin(t + framePhase), 0.002 * cos(t * 0.91 + framePhase * 1.3));
    jitter *= (0.6 + 0.8 * gInst01);
    vec2 uC = mix(TexCoord, kUV, 0.93) + jitter;

    vec4 t1 = textureGrad(textTexture, fract(uA), dFdx(uA), dFdy(uA));
    vec4 t2 = textureGrad(textTexture, fract(uB), dFdx(uB), dFdy(uB));
    vec4 t3 = textureGrad(textTexture, fract(uC), dFdx(uC), dFdy(uC));

    float hueBase = fract(gSlow * 0.04 + chT0 * 0.05 + datePhase * 0.1);
    float sat = 0.65 + 0.25 * sin(gSlow * 0.4 + sr + gAmp01 * 2.0);
    float val = 0.6 + 0.3 * sin(gFast * 0.35 + dot(TexCoord, vec2(2.3, 1.9)) + gInst01 * 3.0);
    vec3 tint = hsv2rgb(vec3(hueBase + r * 0.25, sat, val));

    float slowBeat = 0.5 + 0.5 * sin(gSlow * 0.8 + aMix * 0.05);
    float mix1 = 0.5 + 0.5 * sin(gFast * 0.6 + chT0 * 0.3);
    float mix2 = 0.5 + 0.5 * cos(gSlow * 0.45 + framePhase);

    vec3 warpCol = mix(t1.rgb, t2.rgb, mix1);
    warpCol = mix(warpCol, t3.rgb, mix2 * (0.4 + 0.4 * gAmp01));
    float gain = mix(0.9, 1.5, gAmp01) * (0.9 + 0.3 * slowBeat);
    warpCol *= tint * gain;

    vec3 base = textureGrad(textTexture, TexCoord, dFdx(TexCoord), dFdy(TexCoord)).rgb;

    float warpMix = 0.45 + 0.35 * gAmp01;
    vec3 combined = mix(base, warpCol, warpMix);

    vec3 bloom = combined * combined * 0.08
               + pow(max(combined - 0.6, 0.0), vec3(2.0)) * 0.05;
    combined += bloom;

    combined = limitHighlights(combined);
    combined = clamp(combined, vec3(0.0), vec3(1.0));

    color = vec4(combined, 1.0);
}
)SHD";
inline const char *src_frac_shader02_dmdXi2 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i = 0; i < 6; i++){
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t){
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.05);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

void main(void){
    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);
    vec2 m = (iMouse.z > 0.5 ? (iMouse.xy / iResolution) : vec2(0.5));

    float t = time_f + iTime;
    float sr = clamp(iSampleRate / 48000.0, 0.25, 4.0);
    float aMix = clamp(amp * 0.7 + uamp * 0.3, 0.0, 20.0);

    float seg = 4.0 + 2.0 * sin(t * 0.25);
    float zoom = 1.45 + 0.45 * sin(t * 0.35 + 0.2 * sr);

    vec2 kUV = reflectUV(TexCoord, seg, m, ar.x);
    kUV = fractalFold(kUV, zoom, t, m, ar.x);
    kUV = rotateUV(kUV, t * 0.22, m, ar.x);

    vec2 dir = (kUV - m) * ar;
    float r = length(dir);
    float ripple = sin(20.0 * r - t * 9.0) * 0.012 / (1.0 + 18.0 * r);
    ripple *= 1.0 / (1.0 + 12.0 * (abs(dFdx(r)) + abs(dFdy(r))));

    vec2 uA = kUV + normalize(dir + 1e-5) * ripple;
    vec2 uB = mix(TexCoord, kUV, 0.88);
    vec2 uC = mix(TexCoord, kUV, 0.94) + vec2(0.002 * sin(t), 0.002 * cos(t));

    vec4 t1 = textureGrad(textTexture, fract(uA), dFdx(uA), dFdy(uA));
    vec4 t2 = textureGrad(textTexture, fract(uB), dFdx(uB), dFdy(uB));
    vec4 t3 = textureGrad(textTexture, fract(uC), dFdx(uC), dFdy(uC));

    float hueBase = fract(t * 0.06);
    float sat = 0.7 + 0.25 * sin(t * 0.4 + sr);
    float val = 0.6 + 0.35 * sin(t * 0.5 + dot(TexCoord, vec2(2.3, 1.9)));
    vec3 tint = hsv2rgb(vec3(hueBase + r * 0.25, sat, val));

    float slowBeat = 0.5 + 0.5 * sin(t * 0.8 + aMix * 0.05);
    float mix1 = 0.5 + 0.5 * sin(t * 0.6);
    float mix2 = 0.5 + 0.5 * cos(t * 0.45);

    vec3 warpCol = mix(sin(t1.rgb * pingPong(time_f * PI, 10.0)), sin(t2.rgb * pingPong(time_f * PI/2.0,8.0)), mix1);
    warpCol = mix(warpCol, t3.rgb, mix2 * 0.5);
    warpCol *= tint * (0.92 + 0.08 * slowBeat);
    vec3 base = textureGrad(textTexture, TexCoord, dFdx(TexCoord), dFdy(TexCoord)).rgb;
    vec3 combined = mix(base, warpCol * 3.0, 0.6); 
    vec3 bloom = combined * combined * 0.18 + pow(max(combined - 0.6, 0.0), vec3(2.0)) * 0.10;
    combined += bloom;

    combined = clamp(combined, vec3(0.0), vec3(1.0));
    color = vec4(combined, 1.0);
}
)SHD";
inline const char *src_frac_shader02_dmdXi3 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i = 0; i < 6; i++){
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t){
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.05);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

void main(void){
    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);
    vec2 m = (iMouse.z > 0.5 ? (iMouse.xy / iResolution) : vec2(0.5));

    float t = time_f + iTime * (PI/2.0);
    float sr = clamp(iSampleRate / 48000.0, 0.25, 4.0);
    float aMix = clamp(amp * 0.7 + uamp * 0.3, 0.0, 20.0);

    float seg = 4.0 + 2.0 * sin(t * 0.25);
    float zoom = 1.45 + 0.45 * sin(t * 0.35 + 0.2 * sr);

    vec2 kUV = reflectUV(TexCoord, seg, m, ar.x);
    kUV = fractalFold(kUV, zoom, t, m, ar.x);
    kUV = rotateUV(kUV, t * 0.22, m, ar.x);

    vec2 dir = (kUV - m) * ar;
    float r = length(dir);
    float ripple = sin(20.0 * r - t * 9.0) * 0.012 / (1.0 + 18.0 * r);
    ripple *= 1.0 / (1.0 + 12.0 * (abs(dFdx(r)) + abs(dFdy(r))));
    ripple = sin(ripple * pingPong(time_f * PI, 3.0));

    vec2 uA = kUV + normalize(dir + 1e-5) * ripple;
    vec2 uB = mix(TexCoord, kUV, 0.88);
    vec2 uC = mix(TexCoord, kUV, 0.94) + vec2(0.002 * sin(t), 0.002 * cos(t));

    vec4 t1 = textureGrad(textTexture, fract(uA), dFdx(uA), dFdy(uA));
    vec4 t2 = textureGrad(textTexture, fract(uB), dFdx(uB), dFdy(uB));
    vec4 t3 = textureGrad(textTexture, fract(uC), dFdx(uC), dFdy(uC));

    float hueBase = fract(t * 0.06);
    float sat = 0.7 + 0.25 * sin(t * 0.4 + sr);
    float val = 0.6 + 0.35 * sin(t * 0.5 + dot(TexCoord, vec2(2.3, 1.9)));
    vec3 tint = hsv2rgb(vec3(hueBase + r * 0.25, sat, val));

    float slowBeat = 0.5 + 0.5 * sin(t * 0.8 + aMix * 0.05);
    float mix1 = 0.5 + 0.5 * sin(t * 0.6);
    float mix2 = 0.5 + 0.5 * cos(t * 0.45);

    vec3 warpCol = mix(t1.rgb, t2.rgb, mix1);
    warpCol = mix(warpCol, t3.rgb, mix2 * 0.5);
    warpCol *= tint * (0.92 + 0.08 * slowBeat);
    vec3 base = textureGrad(textTexture, TexCoord, dFdx(TexCoord), dFdy(TexCoord)).rgb;
    vec3 combined = mix(base, warpCol * 3.0, 0.6); 
    vec3 bloom = combined * combined * 0.18 + pow(max(combined - 0.6, 0.0), vec3(2.0)) * 0.10;
    combined += bloom;

    combined = clamp(combined, vec3(0.0), vec3(1.0));
    color = vec4(combined, 1.0);
}
)SHD";
inline const char *src_frac_shader02_dmdXi3_drain = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i = 0; i < 6; i++){
        float z = zoom + 0.15 * sin(t * 0.35 + float(i));
        p = abs((p - c) * z) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

void main(void){
    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);

    vec2 m = (iMouse.z > 0.5 ? (iMouse.xy / iResolution) : vec2(0.5));
    m += (iMouseClick / iResolution) * 0.1;

    float t = time_f + iTime * (PI / 2.0);
    float rate = max(iFrameRate, 1.0);
    float sr = clamp((iSampleRate / 48000.0) * (rate / 60.0), 0.25, 4.0);
    float aMix = clamp(amp * 0.7 + uamp * 0.3, 0.0, 20.0);

    float chanAspect = iChannelResolution[0].y > 0.0 
        ? iChannelResolution[0].x / iChannelResolution[0].y 
        : 1.0;
    float chanBeat = 0.5 + 0.5 * sin(iChannelTime[0] * 0.5 + iDate.x * 0.1);

    float loopDuration = 25.0;
    float currentTime = mod(time_f + float(iFrame) * 0.01, loopDuration);
    float zoomPing = pingPong((time_f + iTimeDelta * 60.0) * 0.25 + chanBeat, 1.0);

    vec2 normCoord = (TexCoord * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
    normCoord.x = abs(normCoord.x);
    float dist = length(normCoord);
    dist = clamp(dist, 0.0, 1.4);

    float angle = atan(normCoord.y, normCoord.x);
    float spiralSpeed = 5.0;
    float inwardBase = currentTime / loopDuration;
    float inwardSpeed = mix(inwardBase, zoomPing, 0.5);

    float edgeFade = 1.0 - smoothstep(0.0, 8.0, dist);
    angle += edgeFade * currentTime * spiralSpeed * (0.6 + 0.4 * chanBeat);

    dist *= 1.0 - inwardSpeed;
    float distWarp = tan(dist * (0.4 + 0.3 * zoomPing));
    vec2 spiralCoord = vec2(cos(angle), sin(angle)) * distWarp;
    spiralCoord = (spiralCoord / vec2(iResolution.x / iResolution.y, 1.0) + 1.0) * 0.5;

    vec2 baseUV = mix(TexCoord, spiralCoord, 0.75);
    baseUV = fract(baseUV);

    float osc = 0.5 + 0.5 * sin(t * 0.35 + 0.2 * sr + iDate.y * 0.01);
    float seg = 4.0 + 2.0 * sin(t * 0.25 + chanAspect * 0.2);
    float zoomBase = mix(1.1, 2.1, osc);
    float zoom = zoomBase * (0.7 + 1.8 * zoomPing);

    float spinAngle = osc * 2.0 * PI * (0.8 + 0.4 * chanBeat);

    vec2 kUV = reflectUV(baseUV, seg, m, ar.x);
    kUV = fractalFold(kUV, zoom, t, m, ar.x);
    kUV = rotateUV(kUV, spinAngle, m, ar.x);

    vec2 dir = (kUV - m) * ar;
    float r = length(dir);

    float ripple = sin(20.0 * r - t * 9.0 - iDate.z * 0.02) * 0.012 / (1.0 + 18.0 * r);
    ripple *= 1.0 / (1.0 + 12.0 * (abs(dFdx(r)) + abs(dFdy(r))));
    ripple = sin(ripple * pingPong(time_f * PI + chanBeat, 3.0 + 1.5 * zoomPing));

    ripple *= 1.0 + 0.6 * sin(aMix * 0.1 + iTimeDelta * 10.0);

    vec2 uA = kUV + normalize(dir + 1e-5) * ripple;
    vec2 uB = mix(baseUV, kUV, 0.88);
    vec2 uC = mix(baseUV, kUV, 0.94) 
              + vec2(0.002 * sin(t + iDate.w * 0.05), 0.002 * cos(t + iTimeDelta * 5.0));

    vec2 fA = fract(uA);
    vec2 fB = fract(uB);
    vec2 fC = fract(uC);

    vec4 t1 = textureGrad(textTexture, fA, dFdx(fA), dFdy(fA));
    vec4 t2 = textureGrad(textTexture, fB, dFdx(fB), dFdy(fB));
    vec4 t3 = textureGrad(textTexture, fC, dFdx(fC), dFdy(fC));

    float hueBase = fract(t * 0.06 + iDate.x * 0.01);
    float sat = 0.7 + 0.25 * sin(t * 0.4 + sr + chanBeat);
    float val = 0.6 + 0.35 * sin(t * 0.5 + dot(baseUV, vec2(2.3, 1.9)) + float(iFrame) * 0.002);

    vec3 tint = hsv2rgb(vec3(hueBase + r * 0.25, sat, val));

    float slowBeat = 0.5 + 0.5 * sin(t * 0.8 + aMix * 0.05 + chanBeat);
    float mix1 = 0.5 + 0.5 * sin(t * 0.6 + zoomPing * PI);
    float mix2 = 0.5 + 0.5 * cos(t * 0.45 + inwardBase * PI);

    vec3 warpCol = mix(t1.rgb, t2.rgb, mix1);
    warpCol = mix(warpCol, t3.rgb, mix2 * 0.5);
    warpCol *= tint * (0.92 + 0.08 * slowBeat);

    vec3 baseTex = textureGrad(textTexture, TexCoord, dFdx(TexCoord), dFdy(TexCoord)).rgb;
    vec3 spiralTex = textureGrad(textTexture, spiralCoord, dFdx(spiralCoord), dFdy(spiralCoord)).rgb;
    vec3 spiralMix = mix(baseTex, spiralTex, 0.7 + 0.3 * zoomPing);

    vec3 combined = mix(spiralMix, warpCol * 3.0, 0.6);

    vec3 bloom = combined * combined * 0.18 
               + pow(max(combined - 0.6, 0.0), vec3(2.0)) * 0.10;
    combined += bloom;

    combined = clamp(combined, vec3(0.0), vec3(1.0));
    color = vec4(combined, 1.0);
}
)SHD";
inline const char *src_frac_shader02_dmdXi3_warp = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 neonPalette(float t){
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.05);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i = 0; i < 6; i++){
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec2 spinWarp(vec2 uv, float t){
    vec2 center = vec2(0.5);
    float angle = atan(uv.y - center.y, uv.x - center.x);
    float modulatedTime = pingPong(t, 5.0);
    angle += modulatedTime;

    vec2 rel = uv - center;
    float s = sin(angle), c = cos(angle);
    vec2 rot;
    rot.x = c * rel.x - s * rel.y;
    rot.y = s * rel.x + c * rel.y;
    vec2 rotated = rot + center;

    float warpFactor = sin(t) * 0.5 + 0.5;
    vec2 warped;
    warped.x = pingPong(rotated.x + t * 0.1, 1.0);
    warped.y = pingPong(rotated.y + t * 0.1, 1.0);

    return mix(rotated, warped, warpFactor);
}

void main(void){
    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);
    vec2 m = (iMouse.z > 0.5 ? (iMouse.xy / iResolution) : vec2(0.5));

    float t = time_f + iTime * (PI / 2.0);
    float sr = clamp(iSampleRate / 48000.0, 0.25, 4.0);
    float aMix = clamp(amp * 0.7 + uamp * 0.3, 0.0, 20.0);

    float osc = 0.5 + 0.5 * sin(t * 0.35 + 0.2 * sr);
    float seg = 4.0 + 2.0 * sin(t * 0.25);
    float zoom = mix(1.1, 2.1, osc);
    float spinAngle = osc * 2.0 * PI;

    vec2 baseUV = spinWarp(TexCoord, t);
    vec2 tcMix = mix(TexCoord, baseUV, 0.75);

    vec2 kUV = reflectUV(tcMix, seg, m, ar.x);
    kUV = fractalFold(kUV, zoom, t, m, ar.x);
    kUV = rotateUV(kUV, spinAngle, m, ar.x);
    kUV = spinWarp(kUV, t * 0.7);

    vec2 dir = (kUV - m) * ar;
    float r = length(dir);

    float ripple = sin(20.0 * r - t * 9.0) * 0.012 / (1.0 + 18.0 * r);
    ripple *= 1.0 / (1.0 + 12.0 * (abs(dFdx(r)) + abs(dFdy(r))));
    ripple = sin(ripple * pingPong(time_f * PI, 3.0));

    vec2 uA = kUV + normalize(dir + 1e-5) * ripple;
    vec2 uB = mix(baseUV, kUV, 0.88);
    vec2 uC = mix(tcMix, kUV, 0.94) + vec2(0.002 * sin(t), 0.002 * cos(t));

    vec4 t1 = textureGrad(textTexture, fract(uA), dFdx(uA), dFdy(uA));
    vec4 t2 = textureGrad(textTexture, fract(uB), dFdx(uB), dFdy(uB));
    vec4 t3 = textureGrad(textTexture, fract(uC), dFdx(uC), dFdy(uC));

    float hueBase = fract(t * 0.06);
    float sat = 0.7 + 0.25 * sin(t * 0.4 + sr);
    float val = 0.6 + 0.35 * sin(t * 0.5 + dot(TexCoord, vec2(2.3, 1.9)));
    vec3 hsvTint = hsv2rgb(vec3(hueBase + r * 0.25, sat, val));
    vec3 neon = neonPalette(hueBase + r * 0.15);
    vec3 tint = normalize(hsvTint * 0.7 + neon * 1.3);

    float slowBeat = 0.5 + 0.5 * sin(t * 0.8 + aMix * 0.05);
    float mix1 = 0.5 + 0.5 * sin(t * 0.6);
    float mix2 = 0.5 + 0.5 * cos(t * 0.45);

    vec3 warpCol = mix(t1.rgb, t2.rgb, mix1);
    warpCol = mix(warpCol, t3.rgb, mix2 * 0.5);
    warpCol *= tint * (0.92 + 0.08 * slowBeat);

    vec3 base = textureGrad(textTexture, baseUV, dFdx(baseUV), dFdy(baseUV)).rgb;
    vec3 combined = mix(base, warpCol * 3.0, 0.6);

    vec3 bloom = combined * combined * 0.18 + pow(max(combined - 0.6, 0.0), vec3(2.0)) * 0.10;
    combined += bloom;

    combined = clamp(combined, vec3(0.0), vec3(1.0));
    color = vec4(combined, 1.0);
}
)SHD";
inline const char *src_frac_shader02_dmdXi4 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i = 0; i < 6; i++){
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec2 inwardSpiral(vec2 uv, vec2 c, float aspect, float t){
    vec2 p = uv - c;
    p.x *= aspect;
    float r = length(p) + 1e-6;
    float ang = atan(p.y, p.x);
    float spin = -0.65 * t;
    float swirl = -1.35 / (1.0 + 10.0 * r);
    ang += spin + swirl;
    float pull = 0.18 / (1.0 + 8.0 * r);
    r = max(r - pull, 0.0);
    vec2 q = vec2(cos(ang), sin(ang)) * r;
    q.x /= aspect;
    return q + c;
}

vec3 satBoost(vec3 c, float s){
    float l = dot(c, vec3(0.2126,0.7152,0.0722));
    return mix(vec3(l), c, 1.0 + s);
}

vec3 vibrance(vec3 c, float v){
    float mx = max(c.r, max(c.g, c.b));
    float mn = min(c.r, min(c.g, c.b));
    float sat = mx - mn;
    float amt = 1.0 + v * (1.0 - sat);
    float l = dot(c, vec3(0.2126,0.7152,0.0722));
    return mix(vec3(l), c, amt);
}

vec3 screen(vec3 a, vec3 b){
    return 1.0 - (1.0 - a)*(1.0 - b);
}

vec3 aces(vec3 x){
    const float a=2.51;
    const float b=0.03;
    const float c=2.43;
    const float d=0.59;
    const float e=0.14;
    x = (x*(a*x+b))/(x*(c*x+d)+e);
    return clamp(x,0.0,1.0);
}

void main(void){
    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);
    vec2 m = (iMouse.z > 0.5 ? (iMouse.xy / iResolution) : vec2(0.5));

    float t = (time_f + iTime);
    float sr = clamp(iSampleRate / 48000.0, 0.25, 4.0);
    float aMix = clamp(amp * 0.7 + uamp * 0.3, 0.0, 20.0);

    float seg = 6.0 + 4.0 * sin(t * 0.25);
    float zoom = 1.65 + 0.55 * sin(t * 0.35 + 0.2 * sr);

    vec2 kUV = reflectUV(TexCoord, seg, m, ar.x);
    kUV = fractalFold(kUV, zoom, t, m, ar.x);
    kUV = rotateUV(kUV, t * 0.28, m, ar.x);
    kUV = inwardSpiral(kUV, m, ar.x, t);

    vec2 dir = (kUV - m) * ar;
    float r = length(dir);
    float ripple = sin(22.0 * r - t * 9.5) * 0.013 / (1.0 + 18.0 * r);
    ripple *= 1.0 / (1.0 + 12.0 * (abs(dFdx(r)) + abs(dFdy(r))));

    vec2 uA = kUV + normalize(dir + 1e-5) * ripple;
    vec2 uB = mix(TexCoord, kUV, 0.94);
    vec2 uC = mix(TexCoord, kUV, 0.985) + vec2(0.0018 * sin(t), 0.0018 * cos(t));

    vec4 t1 = textureGrad(textTexture, fract(uA), dFdx(uA), dFdy(uA));
    vec4 t2 = textureGrad(textTexture, fract(uB), dFdx(uB), dFdy(uB));
    vec4 t3 = textureGrad(textTexture, fract(uC), dFdx(uC), dFdy(uC));

    float hueSweep = fract(t*0.07 + TexCoord.x*0.35 + TexCoord.y*0.18 + r*0.25);
    float hueSweep2 = fract(t*0.05 - TexCoord.y*0.28 + TexCoord.x*0.12 - r*0.18);
    float sat = 0.85 + 0.12*sin(t*0.6 + sr);
    vec3 gradA = hsv2rgb(vec3(hueSweep, sat, 1.0));
    vec3 gradB = hsv2rgb(vec3(hueSweep2, 0.95, 1.0));
    vec3 gradient = mix(gradA, gradB, 0.5 + 0.5*sin(t*0.4 + dot(TexCoord, vec2(1.7,1.3))));

    float slowBeat = 0.5 + 0.5 * sin(t * 0.8 + aMix * 0.05);
    float mix1 = 0.55 + 0.45 * sin(t * 0.6);
    float mix2 = 0.55 + 0.45 * cos(t * 0.45);

    vec3 warpCol = mix(t1.rgb, t2.rgb, mix1);
    warpCol = mix(warpCol, t3.rgb, mix2 * 0.65);

    vec3 neon = screen(warpCol, gradient);
    neon = satBoost(neon, 0.65);
    neon = vibrance(neon, 0.9);
    neon = pow(max(neon, 0.0), vec3(0.9))*(1.15 + 0.1*slowBeat);

    vec3 base = textureGrad(textTexture, TexCoord, dFdx(TexCoord), dFdy(TexCoord)).rgb;
    float patMix = clamp(0.82 + 0.16*slowBeat + 0.12*clamp(aMix*0.08,0.0,1.0), 0.0, 0.98);
    vec3 combined = mix(base, neon, patMix);

    vec3 bloom = combined*combined*0.16 + pow(max(combined-0.7,0.0), vec3(2.0))*0.10;
    combined += bloom;

    combined = aces(combined);
    color = vec4(combined, 1.0);
    color  = mix(color, texture(textTexture, TexCoord), 0.8);
}
)SHD";
inline const char *src_frac_shader02_dmdXi5 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec2 mirror(vec2 uv){
    vec2 w = abs(fract(uv*0.5+0.5)*2.0-1.0);
    return clamp(w, 0.0, 1.0);
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i = 0; i < 6; i++){
        float z = zoom + 0.10 * sin(t * 0.20 + float(i)*0.7);
        p = abs((p - c) * z) - 0.5 + c;
        p = rotateUV(p, t * 0.10 + float(i) * 0.06, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t){
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.04);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.02;
}

vec3 softTone(vec3 c){
    c = pow(max(c, 0.0), vec3(0.96));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res){
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv, float t, float sr){
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    vec2 gdir = normalize(vec2(cos(t * 0.18 + 0.3*sr), sin(t * 0.20 - 0.3*sr)));
    float s = dot((uv - 0.5) * vec2(iResolution.x / iResolution.y, 1.0), gdir);
    float w = max(fwidth(s) * 6.0, 0.003);
    float band = smoothstep(-0.5 - w, -0.5 + w, sin(s * 1.6 + t * 0.6));
    vec3 neon = neonPalette(t);
    vec3 grad = mix(tex, mix(tex, neon, 0.55), 0.30 + 0.20 * band);
    grad = mix(grad, tex, 0.12);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p){
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect){
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if(p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 wormholeUV(vec2 uv, vec2 c, float aspect, float t){
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    float r = length(p) + 1e-6;
    float a = atan(p.y, p.x);
    float swirl = 0.18 / r;
    a += t * 0.25 + swirl;
    float z = 0.55 + 0.30 * sin(t * 0.45) + 0.10 * sin(r * 9.0 - t * 2.0);
    float rr = 1.0 / (r * 3.2 + 0.08) + 0.06 * z;
    vec2 q = vec2(cos(a), sin(a)) * rr;
    q /= ar;
    return mirror(q + c);
}

float sceneLuma(){
    vec3 s0 = texture(textTexture, vec2(0.25,0.25)).rgb;
    vec3 s1 = texture(textTexture, vec2(0.50,0.25)).rgb;
    vec3 s2 = texture(textTexture, vec2(0.75,0.25)).rgb;
    vec3 s3 = texture(textTexture, vec2(0.25,0.50)).rgb;
    vec3 s4 = texture(textTexture, vec2(0.50,0.50)).rgb;
    vec3 s5 = texture(textTexture, vec2(0.75,0.50)).rgb;
    vec3 s6 = texture(textTexture, vec2(0.25,0.75)).rgb;
    vec3 s7 = texture(textTexture, vec2(0.50,0.75)).rgb;
    vec3 s8 = texture(textTexture, vec2(0.75,0.75)).rgb;
    float L = 0.0;
    L += dot(s0, vec3(0.299,0.587,0.114));
    L += dot(s1, vec3(0.299,0.587,0.114));
    L += dot(s2, vec3(0.299,0.587,0.114));
    L += dot(s3, vec3(0.299,0.587,0.114));
    L += dot(s4, vec3(0.299,0.587,0.114));
    L += dot(s5, vec3(0.299,0.587,0.114));
    L += dot(s6, vec3(0.299,0.587,0.114));
    L += dot(s7, vec3(0.299,0.587,0.114));
    L += dot(s8, vec3(0.299,0.587,0.114));
    return L / 9.0;
}

vec3 toneDownWhite(vec3 c){
    float lum = dot(c, vec3(0.299, 0.587, 0.114));
    float factor = smoothstep(0.6, 1.0, lum);
    c = mix(c, c * 0.7, factor);
    c = pow(c, vec3(0.95 + 0.05 * (1.0 - factor)));
    return clamp(c, 0.0, 1.0);
}

void main(void){
    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);
    vec2 m = (iMouse.z > 0.5 ? (iMouse.xy / iResolution) : vec2(0.5));
    float t = time_f + iTime;
    float sr = clamp(iSampleRate / 48000.0, 0.25, 4.0);
    float aMix = clamp(amp * 0.7 + uamp * 0.3, 0.0, 20.0);

    float seg = 4.0 + 2.0 * sin(t * 0.20 + 0.25 * aMix);
    float zoom = 1.45 + 0.40 * sin(t * 0.28 + 0.15 * sr);

    vec2 kUV = reflectUV(TexCoord, seg, m, ar.x);
    kUV = fractalFold(kUV, zoom, t, m, ar.x);
    kUV = rotateUV(kUV, t * 0.18, m, ar.x);
    kUV = diamondFold(kUV, m, ar.x);

    vec2 dir = (kUV - m) * ar;
    float r = length(dir);
    float ripple = sin(14.0 * r - t * 5.0) * 0.008 / (1.0 + 12.0 * r);
    ripple *= 1.0 / (1.0 + 10.0 * (abs(dFdx(r)) + abs(dFdy(r))));
    vec2 uA = kUV + normalize(dir + 1e-5) * ripple;
    vec2 uB = mix(TexCoord, kUV, 0.82);
    vec2 uC = mix(TexCoord, kUV, 0.90) + vec2(0.0015 * sin(t*0.8), 0.0015 * cos(t*0.8));

    vec2 sA = mirror(uA);
    vec2 sB = mirror(uB);
    vec2 sC = mirror(uC);

    vec4 t1 = textureGrad(textTexture, sA, dFdx(sA), dFdy(sA));
    vec4 t2 = textureGrad(textTexture, sB, dFdx(sB), dFdy(sB));
    vec4 t3 = textureGrad(textTexture, sC, dFdx(sC), dFdy(sC));

    vec3 warpCol = mix(t1.rgb, t2.rgb, 0.5 + 0.5 * sin(t * 0.45));
    warpCol = mix(warpCol, t3.rgb, 0.25 + 0.25 * cos(t * 0.33));

    float sat = 0.72 + 0.22 * sin(t * 0.32 + sr);
    float val = 0.62 + 0.30 * sin(t * 0.36 + dot(TexCoord, vec2(2.1, 1.7)));
    vec3 tint = hsv2rgb(vec3(fract(t * 0.045) + r * 0.20, sat, val));
    warpCol *= tint * (0.94 + 0.06 * (0.5 + 0.5 * sin(t * 0.6 + aMix * 0.04)));

    vec3 baseTex = textureGrad(textTexture, TexCoord, dFdx(TexCoord), dFdy(TexCoord)).rgb;
    vec3 preK = preBlendColor(mirror(kUV), t, sr);

    vec2 wh0 = wormholeUV(TexCoord, m, ar.x, t);
    vec2 wh1 = wormholeUV(TexCoord + vec2(0.0009, 0.0), m, ar.x, t + 0.02);
    vec2 wh2 = wormholeUV(TexCoord - vec2(0.0009, 0.0), m, ar.x, t - 0.02);
    vec2 off = normalize(dir + 1e-5) * (0.0012 + 0.0008 * sin(t * 0.9)) * vec2(1.0, 1.0 / ar.x);

    vec3 whR = preBlendColor(mirror(wh0 + off), t, sr);
    vec3 whG = preBlendColor(mirror(wh1), t, sr);
    vec3 whB = preBlendColor(mirror(wh2 - off), t, sr);
    vec3 wormRGB = vec3(sin(whR.r * pingPong(t * PI, 1.1)),
                        sin(whG.g * pingPong(t * PI, 1.2)),
                        sin(whB.b * pingPong(t * PI, 1.3)));

    float rCenter = length((TexCoord - m) * ar);
    float throatBase = smoothstep(0.45, 0.10, rCenter);
    float throat = mix(throatBase, throatBase*throatBase, 0.5) * (0.55 + 0.45 * sin(t * 0.35));
    float swirlGate = smoothstep(0.8, 1.6, t * 0.18 + 0.25 * sin(t * 0.5));
    float gateRaw = throat * (0.55 + 0.35 * pingPong(t * PI, 3.0)) + swirlGate * 0.12;
    float wGate = max(fwidth(gateRaw)*2.0, 0.002);
    float gate = smoothstep(0.0, 1.0, clamp(gateRaw, 0.0, 1.0));
    gate = smoothstep(0.0, 1.0, mix(gate, gate, 1.0 - wGate));

    vec3 mixA = mix(preK, warpCol, 0.52);
    vec3 mixB = mix(mixA, wormRGB, gate * 0.72);

    vec3 bloom = mixB * mixB * 0.14 + pow(max(mixB - 0.65, 0.0), vec3(2.0)) * 0.08;
    vec3 combined = mixB + bloom;

    float vign = 1.0 - smoothstep(0.78, 1.10, length((TexCoord - m) * ar));
    combined *= mix(0.96, 1.06, vign);

    float Lscene = sceneLuma();
    float baseFactor = mix(1.0, 0.75, smoothstep(0.55, 0.9, Lscene));
    float finalFactor = mix(1.0, 0.72, smoothstep(0.55, 0.9, Lscene));

    baseTex = toneDownWhite(baseTex) * baseFactor;

    vec3 master = mix(baseTex, combined * 2.6, 0.58) * finalFactor;
    master = clamp(master, vec3(0.03), vec3(0.97));

    color = vec4(master, 1.0);
}
)SHD";
inline const char *src_frac_shader02_prisim = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i = 0; i < 6; i++){
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t){
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.05);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 warpTexture(vec2 baseUV, float seg, float zoom, float t, vec2 m, float aspect, float aMix, float sr, float spinAngle){
    vec2 ar = vec2(aspect, 1.0);
    vec2 kUV = reflectUV(baseUV, seg, m, aspect);
    kUV = fractalFold(kUV, zoom, t, m, aspect);
    kUV = rotateUV(kUV, spinAngle, m, aspect);

    vec2 dir = (kUV - m) * ar;
    float r = length(dir);

    float ripple = sin(20.0 * r - t * 9.0) * 0.012 / (1.0 + 18.0 * r);
    ripple *= 1.0 / (1.0 + 12.0 * (abs(dFdx(r)) + abs(dFdy(r))));
    ripple = sin(ripple * pingPong(time_f * PI, 3.0));

    vec2 nDir = normalize(dir + 1e-5);
    vec2 uA = kUV + nDir * ripple;
    vec2 uB = mix(baseUV, kUV, 0.88);
    vec2 uC = mix(baseUV, kUV, 0.94) + vec2(0.002 * sin(t), 0.002 * cos(t));

    vec4 t1 = textureGrad(textTexture, fract(uA), dFdx(uA), dFdy(uA));
    vec4 t2 = textureGrad(textTexture, fract(uB), dFdx(uB), dFdy(uB));
    vec4 t3 = textureGrad(textTexture, fract(uC), dFdx(uC), dFdy(uC));

    float ampNorm = clamp(aMix * 0.12, 0.0, 1.5);
    float hueBase = fract(t * 0.06 + r * 0.22 + ampNorm * 0.3);
    float sat = 0.75 + 0.20 * sin(t * 0.4 + sr * 0.7 + ampNorm * 0.4);
    float val = 0.70 + 0.35 * sin(t * 0.55 + r * 1.9 + ampNorm * 0.5);

    vec3 tint = hsv2rgb(vec3(hueBase, sat, val));
    vec3 neon = neonPalette(t + r * 2.0 + ampNorm);

    float mix1 = 0.5 + 0.5 * sin(t * 0.6 + ampNorm * 0.3);
    float mix2 = 0.5 + 0.5 * cos(t * 0.45 + ampNorm * 0.5);

    vec3 warpCol = mix(t1.rgb, t2.rgb, mix1);
    warpCol = mix(warpCol, t3.rgb, mix2 * 0.7);
    warpCol *= tint * neon;
    warpCol *= 1.1 + 0.7 * clamp(aMix * 0.1, 0.0, 1.0);

    vec3 baseTex = textureGrad(textTexture, baseUV, dFdx(baseUV), dFdy(baseUV)).rgb;
    float warpMix = 0.55 + 0.35 * clamp(aMix * 0.08, 0.0, 1.0);
    vec3 combined = mix(baseTex, warpCol * 1.9, warpMix);

    vec3 bloom = combined * combined * 0.12 + pow(max(combined - 0.6, 0.0), vec3(2.0)) * 0.08;
    combined += bloom;

    return combined;
}

void main(void){
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5 ? (iMouse.xy / iResolution) : vec2(0.5));

    float t = time_f + iTime * (PI / 2.0);
    float sr = clamp(iSampleRate / 48000.0, 0.25, 4.0);
    float aMix = clamp(amp * 0.7 + uamp * 0.3, 0.0, 20.0);
    float ampNorm = clamp(aMix * 0.12, 0.0, 1.5);

    float osc = 0.5 + 0.5 * sin(t * 0.35 + 0.2 * sr + ampNorm * 0.2);
    float seg = 4.0 + 2.0 * sin(t * 0.25) + 2.5 * clamp(ampNorm, 0.0, 2.0);
    float zoom = mix(1.1, 2.3 + ampNorm * 0.4, osc);
    float spinAngle = osc * 2.0 * PI * (0.6 + 0.4 * clamp(ampNorm, 0.0, 1.5));

    vec2 centerPx = iResolution * 0.5;
    vec2 texCoordPx = TexCoord * iResolution;
    vec2 deltaPx = texCoordPx - centerPx;
    float dist = length(deltaPx);

    float radius = 0.45 + 0.3 * clamp(ampNorm * 0.7, 0.0, 1.0);
    float maxRadius = min(iResolution.x, iResolution.y) * radius;

    float scaleFactor = 1.0 - pow(clamp(dist / maxRadius, 0.0, 1.0), 2.0);
    scaleFactor *= 1.0 + 0.9 * clamp(ampNorm, 0.0, 1.5);
    scaleFactor = clamp(scaleFactor, 0.0, 1.9);

    vec2 dirN = dist > 0.0 ? deltaPx / dist : vec2(0.0);

    float offsetBase = mix(0.008, 0.03, clamp(ampNorm * 0.6, 0.0, 1.0));
    float offsetR = offsetBase;
    float offsetG = 0.0;
    float offsetB = -offsetBase;

    vec3 colBase = warpTexture(TexCoord, seg, zoom, t, m, aspect, aMix, sr, spinAngle);

    vec3 colBubble = colBase;
    if(dist < maxRadius){
        vec2 texCoordR = centerPx + deltaPx * scaleFactor + dirN * offsetR * maxRadius;
        vec2 texCoordG = centerPx + deltaPx * scaleFactor + dirN * offsetG * maxRadius;
        vec2 texCoordB = centerPx + deltaPx * scaleFactor + dirN * offsetB * maxRadius;

        vec2 uvR = clamp(texCoordR / iResolution, 0.0, 1.0);
        vec2 uvG = clamp(texCoordG / iResolution, 0.0, 1.0);
        vec2 uvB = clamp(texCoordB / iResolution, 0.0, 1.0);

        vec3 colR = warpTexture(uvR, seg, zoom, t, m, aspect, aMix, sr, spinAngle);
        vec3 colG = warpTexture(uvG, seg, zoom, t, m, aspect, aMix, sr, spinAngle);
        vec3 colB = warpTexture(uvB, seg, zoom, t, m, aspect, aMix, sr, spinAngle);

        vec3 rgbSplit = vec3(colR.r, colG.g, colB.b);

        float mask = smoothstep(maxRadius, maxRadius * 0.65, dist);
        colBubble = mix(colBase, rgbSplit, mask);
    }

    vec3 baseVideo = textureGrad(textTexture, TexCoord, dFdx(TexCoord), dFdy(TexCoord)).rgb;
    float globalMix = 0.50 + 0.35 * clamp(ampNorm * 0.5, 0.0, 1.0);
    vec3 combined = mix(baseVideo, colBubble, globalMix);

    combined = combined / (1.0 + combined);
    combined = clamp(combined, vec3(0.0), vec3(1.0));

    color = vec4(combined, 1.0);
}
)SHD";
inline const char *src_frac_shader03_size = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
float pingPong(float x, float length){
    float m = mod(x, length*2.0);
    return m <= length ? m : length*2.0 - m;
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i=0;i<6;i++){
        p = abs((p - c) * (zoom + 0.15*sin(t*0.35+float(i)))) - 0.5 + c;
        p = rotateUV(p, t*0.12 + float(i)*0.07, c, aspect);
    }
    return p;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 movingGradient(vec2 uv, vec2 c, float t, float aspect){
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    vec2 d = normalize(vec2(cos(t*0.27), sin(t*0.31)));
    float s = dot(p, d);
    float band = 0.5 + 0.5*sin(s*6.28318530718*0.35 + t*0.9);
    float h = fract(s*0.22 + t*0.07 + 0.15*sin(t*0.33));
    float S = 0.75 + 0.25*sin(t*0.21 + s*2.0);
    float V = 0.75 + 0.25*band;
    vec3 base = hsv2rgb(vec3(h, S, V));
    float edge = smoothstep(0.2, 0.8, band);
    return mix(base*0.6, base, edge);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect){
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if(p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

float diamondRadius(vec2 p){
    p = abs(p);
    return max(p.x, p.y);
}

float hash(vec2 p){
    return fract(sin(dot(p, vec2(127.1,311.7))) * 43758.5453123);
}

float starWeight(vec2 uv, float grid, float t, float aspect, float cls){
    vec2 g = uv * grid;
    vec2 id = floor(g);
    vec2 f = fract(g) - 0.5;
    float r = hash(id + cls*13.37);
    float sz = mix(0.35, 0.9, r);
    float d = max(abs(f.x)*aspect, abs(f.y));
    float fall = smoothstep(sz*0.9, 0.0, d);
    float tw = 0.5 + 0.5*sin(t*(2.0+6.0*r) + r*6.2831853);
    return fall * tw;
}

void main(){
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = TexCoord;
    vec4 originalTexture = texture(textTexture, TexCoord);

    float seg = 4.0 + 2.0*sin(time_f*0.33);
    vec2 kUV = reflectUV(uv, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if(q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18*sin(time_f*0.2);
    float period = log(base);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35*sin(rD*18.0 + time_f*0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);

    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap*1.045) / ar + m);
    vec2 u2 = fract((pwrap*0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f*1.3)) * vec2(1.0, 1.0/aspect);

    float hue = fract(ang*0.25 + time_f*0.08 + k*0.5);
    float sat = 0.75 - 0.25*cos(time_f*0.7 + rD*10.0);
    float val = 0.8 + 0.2*sin(time_f*0.9 + k*6.28318530718);
    vec3 tint = hsv2rgb(vec3(hue, sat, val));

    float ring = smoothstep(0.0, 0.7, sin(log(rD+1e-3)*9.5 + time_f*1.2));
    float pulse = 0.5 + 0.5*sin(time_f*2.0 + rD*28.0 + k*12.0);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m)*ar));
    vign = mix(0.85, 1.2, vign);

    float blendFactor = 0.58;

    float rC = texture(textTexture, u0 + off).r;
    float gC = texture(textTexture, u1).g;
    float bC = texture(textTexture, u2 - off).b;
    vec3 baseRGB = vec3(rC, gC, bC);

    vec2 s1p = vec2(cos(ang), sin(ang)) * (rw*0.55);
    vec2 s2p = vec2(cos(ang), sin(ang)) * (rw*1.10);
    vec2 s3p = vec2(cos(ang), sin(ang)) * (rw*1.85);

    vec3 sample1 = texture(textTexture, fract(s1p/ar + m)).rgb;
    vec3 sample2 = texture(textTexture, fract(s2p/ar + m)).rgb;
    vec3 sample3 = texture(textTexture, fract(s3p/ar + m)).rgb;

    float wSmall  = starWeight(TexCoord, 38.0, time_f, aspect, 1.0);
    float wMedium = starWeight(TexCoord, 22.0, time_f, aspect, 2.0);
    float wLarge  = starWeight(TexCoord, 12.0, time_f, aspect, 3.0);
    float wSum = max(1e-4, wSmall + wMedium + wLarge);

    vec3 twinkleMix = (sample1*wSmall + sample2*wMedium + sample3*wLarge) / wSum;

    vec3 kaleido = baseRGB * tint;
    vec3 merged = mix(kaleido, originalTexture.rgb, blendFactor);

    merged *= (0.75 + 0.25*ring) * (0.85 + 0.15*pulse) * vign;

    vec3 bloom = merged * merged * 0.18 + pow(max(merged-0.6,0.0), vec3(2.0))*0.12;
    vec3 outCol = merged + bloom;

    outCol = mix(outCol, twinkleMix, 0.35 + 0.25*sin(time_f*0.6 + rD*7.0 + k*11.0));

    float wob = 0.9 + 0.1*sin(time_f + rD*14.0 + k*9.0);
    outCol *= wob;

    vec3 grad = movingGradient(TexCoord, m, time_f, aspect);
    float gradAmt = 0.25 + 0.25*sin(time_f*0.5 + rD*7.0 + k*9.0);
    vec3 screenBlend = 1.0 - (1.0 - outCol) * (1.0 - grad);
    outCol = mix(outCol, screenBlend, gradAmt);

    vec4 t = texture(textTexture, TexCoord);
    outCol = mix(outCol, outCol*t.rgb, 0.8);

    outCol = sin(outCol * (0.5 + 0.5*pingPong(time_f, 12.0)));
    outCol = clamp(outCol, vec3(0.08), vec3(0.96));

    color = vec4(outCol, 1.0);
}
)SHD";
inline const char *src_frac_shader03_wormhole = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 wormholeUV(vec2 uv, vec2 c, float aspect, float t) {
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    float r = length(p) + 1e-6;
    float a = atan(p.y, p.x);
    float swirl = 0.22 / r;
    a += t * 0.35 + swirl;
    float z = 0.65 + 0.35 * sin(t * 0.6) + 0.15 * sin(r * 12.0 - t * 3.0);
    float rr = 1.0 / (r * 3.5 + 0.06) + 0.08 * z;
    vec2 q = vec2(cos(a), sin(a)) * rr;
    q /= ar;
    return fract(q + c);
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;

    vec2 wh0 = wormholeUV(TexCoord, m, aspect, time_f);
    vec2 wh1 = wormholeUV(TexCoord + vec2(0.0009, 0.0), m, aspect, time_f + 0.03);
    vec2 wh2 = wormholeUV(TexCoord - vec2(0.0009, 0.0), m, aspect, time_f - 0.03);
    vec3 whR = preBlendColor(wh0 + off);
    vec3 whG = preBlendColor(wh1);
    vec3 whB = preBlendColor(wh2 - off);
    vec3 wormRGB = vec3(whR.r, whG.g, whB.b);

    float rCenter = length((TexCoord - m) * ar);
    float throat = smoothstep(0.38, 0.06, rCenter);
    float swirlGate = smoothstep(0.9, 1.6, time_f * 0.25 + 0.35 * sin(time_f * 0.7));
    float gate = clamp(throat * (0.65 + 0.35 * pingPong(time_f * PI, 5.0)) + swirlGate * 0.15, 0.0, 1.0);

    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = mix(outCol, wormRGB, gate * 0.85);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader03_wormhole_amp = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;

const float PI = 3.1415926535897932384626433832795;

float gAmp01;
float gSlow;
float gFast;
float gDetail;
float gInstAmp;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    float f = mix(0.5, 2.0, gAmp01);
    p = sin(abs(p * f));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 wormholeUV(vec2 uv, vec2 c, float aspect, float t) {
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    float r = length(p) + 1e-6;
    float a = atan(p.y, p.x);

    float swirl = 0.22 / r;

    float spinBase = 0.15 + 0.35 * gAmp01;
    float spinHit  = 0.2 + 2.2 * gInstAmp;

    a += t * (spinBase + spinHit) + swirl;

    float z = 0.65 + 0.35 * sin(t * 0.6)
              + 0.15 * sin(r * 12.0 - t * (2.5 + 1.5 * gAmp01 + 2.0 * gInstAmp));
    float rr = 1.0 / (r * 3.5 + 0.06) + 0.08 * z;

    vec2 q = vec2(cos(a), sin(a)) * rr;
    q /= ar;
    return fract(q + c);
}

vec3 limitHighlights(vec3 c) {
    float m = max(c.r, max(c.g, c.b));
    if (m > 0.9) c *= 0.9 / m;
    return c;
}

void main(void) {
    float aAcc = clamp(amp, 0.0, 4.0);
    float aInst = clamp(uamp, 0.0, 4.0);
    float ampMix = clamp(aAcc * 0.6 + aInst * 1.4, 0.0, 4.0);
    gAmp01 = clamp(ampMix / 2.5, 0.0, 1.0);
    gInstAmp = clamp(aInst / 2.5, 0.0, 1.0);

    gSlow   = time_f * mix(0.15, 0.7, gAmp01);
    gFast   = time_f * mix(0.6,  3.5, gAmp01);
    gDetail = time_f * mix(0.3,  2.0, gAmp01);

    vec4 baseTex = texture(textTexture, TexCoord);

    float aspect = iResolution.x / iResolution.y;
    vec2 uv = TexCoord * 2.0 - 1.0;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * gSlow), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);

    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);

    vec3 baseCol = preBlendColor(TexCoord);

    float seg = 4.0 + 2.0 * sin(gSlow * 0.33 + gAmp01 * 2.0);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    float foldZoom = 1.45 + 0.55 * sin(gSlow * 0.42 + gAmp01 * 3.0);
    kUV = fractalFold(kUV, foldZoom, gDetail, m, aspect);
    kUV = rotateUV(kUV, gSlow * 0.23 + gAmp01 * 1.1, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(gSlow * 0.2) * (PI * gSlow), 5.0);
    float period = log(base) * pingPong(gSlow * PI, 5.0);
    float tz = gSlow * 0.65;
    float rD = diamondRadius(p) + 1e-6;

    float ang = atan(q.y, q.x) + tz * 0.35
              + 0.35 * sin(rD * 18.0 + gFast * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(gFast * 1.3))
              * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + gFast * 1.2));
    ring = ring * pingPong(gSlow * PI, 5.0);

    float pulse = 0.5 + 0.5 * sin(gFast * 2.0 + rD * 28.0 + k * 12.0);
    pulse *= mix(0.4, 1.4, gAmp01);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.10
               + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.07;

    vec2 wh0 = wormholeUV(TexCoord,                    m, aspect, gSlow);
    vec2 wh1 = wormholeUV(TexCoord + vec2(0.0009, 0.0), m, aspect, gSlow + 0.03);
    vec2 wh2 = wormholeUV(TexCoord - vec2(0.0009, 0.0), m, aspect, gSlow - 0.03);

    vec3 whR = preBlendColor(wh0 + off);
    vec3 whG = preBlendColor(wh1);
    vec3 whB = preBlendColor(wh2 - off);
    vec3 wormRGB = vec3(whR.r, whG.g, whB.b);

    float rCenter = length((TexCoord - m) * ar);
    float throat = smoothstep(0.38, 0.06, rCenter);

    float swirlGate = smoothstep(0.9, 1.6,
        gSlow * 0.25 + 0.35 * sin(gSlow * 0.7));

    float gateBase = clamp(throat * (0.55 + 0.45 * pingPong(gSlow * PI, 5.0))
                           + swirlGate * 0.15, 0.0, 1.0);

    float hitBoost = clamp(aInst * 0.6, 0.0, 1.2);
    float gate = clamp(gateBase * (0.6 + 0.9 * gAmp01) + hitBoost, 0.0, 1.0);

    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = mix(outCol, wormRGB, gate * 0.85);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    outCol *= mix(0.85, 1.20, gAmp01);

    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    finalRGB = limitHighlights(finalRGB);
    finalRGB = clamp(finalRGB, 0.0, 1.0);

    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader03_wormhole2 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 wormholeUV(vec2 uv, vec2 c, float aspect, float t) {
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    float r = length(p) + 1e-6;
    float a = atan(p.y, p.x);
    float swirl = 0.22 / r;
    a += t * 0.35 + swirl;
    float z = 0.65 + 0.35 * sin(t * 0.6) + 0.15 * sin(r * 12.0 - t * 3.0);
    float rr = 1.0 / (r * 3.5 + 0.06) + 0.08 * z;
    vec2 q = vec2(cos(a), sin(a)) * rr;
    q /= ar;
    return fract(q + c);
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;

    vec2 wh0 = wormholeUV(TexCoord, m, aspect, pingPong((time_f * PI), 5.0));
    vec2 wh1 = wormholeUV(TexCoord + vec2(0.0009, 0.0), m, aspect, time_f + 0.03);
    vec2 wh2 = wormholeUV(TexCoord - vec2(0.0009, 0.0), m, aspect, time_f - 0.03);
    vec3 whR = preBlendColor(wh0 + off);
    vec3 whG = preBlendColor(wh1);
    vec3 whB = preBlendColor(wh2 - off);
    vec3 wormRGB = vec3(whR.r, whG.g, whB.b);

    float rCenter = length((TexCoord - m) * ar);
    float d00 = length(ar * (vec2(0.0, 0.0) - m));
    float d10 = length(ar * (vec2(1.0, 0.0) - m));
    float d01 = length(ar * (vec2(0.0, 1.0) - m));
    float d11 = length(ar * (vec2(1.0, 1.0) - m));
    float cornerR = max(max(d00, d10), max(d01, d11));
    float phase = pingPong(time_f * 0.6, 1.0);
    float holeR = mix(max(1.5 / max(iResolution.x, iResolution.y), 0.0008), cornerR, phase);
    float rimW = mix(0.02, 0.12, smoothstep(0.0, 1.0, phase)) * cornerR;
    float gate = smoothstep(holeR, holeR - rimW, rCenter);

    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = mix(outCol, wormRGB, gate * 0.95);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader03_wormhole3 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 wormholeUV(vec2 uv, vec2 c, float aspect, float t) {
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    float r = length(p) + 1e-6;
    float a = atan(p.y, p.x);
    float swirl = 0.22 / r;
    a += t * 0.35 + swirl;
    float z = 0.65 + 0.35 * sin(t * 0.6) + 0.15 * sin(r * 12.0 - t * 3.0);
    float rr = 1.0 / (r * 3.5 + 0.06) + 0.08 * z;
    vec2 q = vec2(cos(a), sin(a)) * rr;
    q /= ar;
    return fract(q + c);
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;

    vec2 wh0 = wormholeUV(TexCoord, m, aspect, pingPong((time_f * PI), 5.0));
    vec2 wh1 = wormholeUV(TexCoord + vec2(0.0009, 0.0), m, aspect, time_f + 0.03);
    vec2 wh2 = wormholeUV(TexCoord - vec2(0.0009, 0.0), m, aspect, time_f - 0.03);
    vec3 whR = preBlendColor(wh0 + off);
    vec3 whG = preBlendColor(wh1);
    vec3 whB = preBlendColor(wh2 - off);
    vec3 wormRGB = vec3(whR.r, whG.g, whB.b);

    outCol += bloom;
    outCol = wormRGB;

    vec3 finalRGB = outCol;
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader03_wormhole4 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 wormholeUV(vec2 uv, vec2 c, float aspect, float t) {
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    float r = length(p) + 1e-6;
    float a = atan(p.y, p.x);
    float swirl = 0.22 / r;
    a += t * 0.35 + swirl;
    float z = 0.65 + 0.35 * sin(t * 0.6) + 0.15 * sin(r * 12.0 - t * 3.0);
    float rr = 1.0 / (r * 3.5 + 0.06) + 0.08 * z;
    vec2 q = vec2(cos(a), sin(a)) * rr;
    q /= ar;
    return fract(q + c);
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);

    

    vec3 kaleidoRGB = vec3(sin(rC.r * pingPong(time_f * PI, 3.0)), sin(gC.g * pingPong(time_f * PI, 3.0)), sin(bC.b * pingPong(time_f * PI, 3.0)));
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;

    vec2 wh0 = wormholeUV(TexCoord, m, aspect, time_f);
    vec2 wh1 = wormholeUV(TexCoord + vec2(0.0009, 0.0), m, aspect, time_f + 0.03);
    vec2 wh2 = wormholeUV(TexCoord - vec2(0.0009, 0.0), m, aspect, time_f - 0.03);
    vec3 whR = preBlendColor(wh0 + off);
    vec3 whG = preBlendColor(wh1);
    vec3 whB = preBlendColor(wh2 - off);
    vec3 wormRGB = vec3(sin(whR.r * pingPong(time_f * PI, 1.3)), sin(whG.g * pingPong(time_f * PI, 1.4)), sin(whB.b * pingPong(time_f * PI, 1.5)));

    float rCenter = length((TexCoord - m) * ar);
    float throat = sin(smoothstep(0.38, 0.06, rCenter) * pingPong(time_f * PI, 5.0));
    float swirlGate = smoothstep(0.9, 1.6, time_f * 0.25 + 0.35 * sin(time_f * 0.7)) * pingPong(time_f * PI, 4.0);
    float gate = clamp(throat * (0.65 + 0.35 * pingPong(time_f * PI, 5.0)) + pingPong(swirlGate * PI, 8.0) * 0.15, 0.0, 1.0);

    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = mix(outCol, wormRGB, gate * 0.85);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader04_echo = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i = 0; i < 6; i++){
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t){
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.05);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

void main(void){
    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);
    vec2 m = (iMouse.z > 0.5 ? (iMouse.xy / iResolution) : vec2(0.5));

    float t = time_f + iTime;
    float sr = clamp(iSampleRate / 48000.0, 0.25, 4.0);
    float aMix = clamp(amp * 0.7 + uamp * 0.3, 0.0, 20.0);

    float seg = 4.0 + 2.0 * sin(t * 0.25);
    float zoom = 1.45 + 0.45 * sin(t * 0.35 + 0.2 * sr);

    vec2 kUV = reflectUV(TexCoord, seg, m, ar.x);
    kUV = fractalFold(kUV, zoom, t, m, ar.x);
    kUV = rotateUV(kUV, t * 0.22, m, ar.x);

    vec2 dir = (kUV - m) * ar;
    float r = length(dir);
    float ripple = sin(20.0 * r - t * 9.0) * 0.012 / (1.0 + 18.0 * r);
    ripple *= 1.0 / (1.0 + 12.0 * (abs(dFdx(r)) + abs(dFdy(r))));

    vec2 uA = kUV + normalize(dir + 1e-5) * ripple;
    vec2 uB = mix(TexCoord, kUV, 0.88);
    vec2 uC = mix(TexCoord, kUV, 0.94) + vec2(0.002 * sin(t), 0.002 * cos(t));

    vec4 t1 = textureGrad(textTexture, fract(uA), dFdx(uA), dFdy(uA));
    vec4 t2 = textureGrad(textTexture, fract(uB), dFdx(uB), dFdy(uB));
    vec4 t3 = textureGrad(textTexture, fract(uC), dFdx(uC), dFdy(uC));

    float hueBase = fract(t * 0.06);
    float sat = 0.7 + 0.25 * sin(t * 0.4 + sr);
    float val = 0.6 + 0.35 * sin(t * 0.5 + dot(TexCoord, vec2(2.3, 1.9)));
    vec3 tint = hsv2rgb(vec3(hueBase + r * 0.25, sat, val));

    float slowBeat = 0.5 + 0.5 * sin(t * 0.8 + aMix * 0.05);
    float mix1 = 0.5 + 0.5 * sin(t * 0.6);
    float mix2 = 0.5 + 0.5 * cos(t * 0.45);

    vec3 warpCol = mix(sin(t1.rgb * pingPong(time_f * PI, 10.0)), sin(t2.rgb * pingPong(time_f * PI / 2.0, 8.0)), mix1);
    warpCol = mix(warpCol, t3.rgb, mix2 * 0.5);
    warpCol *= tint * (0.92 + 0.08 * slowBeat);

    vec2 dtx = dFdx(TexCoord);
    vec2 dty = dFdy(TexCoord);

    vec3 base = textureGrad(textTexture, TexCoord, dtx, dty).rgb;

    vec4 s1 = textureGrad(textTexture, TexCoord, dtx, dty);
    vec4 s2 = textureGrad(textTexture, TexCoord * 0.5, dtx * 0.5, dty * 0.5);
    vec4 s3 = textureGrad(textTexture, TexCoord * 0.25, dtx * 0.25, dty * 0.25);
    vec4 s4 = textureGrad(textTexture, TexCoord * 0.125, dtx * 0.125, dty * 0.125);
    vec3 multi = (s1.rgb + s2.rgb + s3.rgb + s4.rgb) * 0.25;

    vec3 combined = mix(base, warpCol * 3.0, 0.6);

    float multiMix = 0.45 + 0.35 * sin(t * 0.25 + aMix * 0.02);
    vec3 multiBlend = mix(base, multi, 0.7);
    combined = mix(combined, multiBlend, multiMix);

    vec3 bloom = combined * combined * 0.18 + pow(max(combined - 0.6, 0.0), vec3(2.0)) * 0.10;
    combined += bloom;

    combined = clamp(combined, vec3(0.0), vec3(1.0));
    color = vec4(combined, 1.0);
}
)SHD";
inline const char *src_frac_shader04_echo2 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect, float spin){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x) + spin;
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i = 0; i < 6; i++){
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t){
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.05);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

void main(void){
    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);
    vec2 m = (iMouse.z > 0.5 ? (iMouse.xy / iResolution) : vec2(0.5));

    float t = time_f + iTime;
    float sr = clamp(iSampleRate / 48000.0, 0.25, 4.0);
    float aMix = clamp(amp * 0.7 + uamp * 0.3, 0.0, 20.0);

    float seg = 4.0 + 2.0 * sin(t * 0.25);
    float zoom = 1.45 + 0.45 * sin(t * 0.35 + 0.2 * sr);
    float kaleidoSpin = t * 0.5 + aMix * 0.05;

    vec2 kUV = reflectUV(TexCoord, seg, m, ar.x, kaleidoSpin);
    kUV = fractalFold(kUV, zoom, t, m, ar.x);

    float radial = length((kUV - m) * ar);
    float globalSpin = t * 0.6;
    kUV = rotateUV(kUV, globalSpin + radial * 4.0, m, ar.x);

    vec2 dir = (kUV - m) * ar;
    float r = length(dir);
    float ripple = sin(20.0 * r - t * 9.0) * 0.012 / (1.0 + 18.0 * r);
    ripple *= 1.0 / (1.0 + 12.0 * (abs(dFdx(r)) + abs(dFdy(r))));

    vec2 uA = kUV + normalize(dir + 1e-5) * ripple;
    vec2 uB = mix(TexCoord, kUV, 0.88);
    vec2 uC = mix(TexCoord, kUV, 0.94) + vec2(0.002 * sin(t), 0.002 * cos(t));

    vec4 t1 = textureGrad(textTexture, fract(uA), dFdx(uA), dFdy(uA));
    vec4 t2 = textureGrad(textTexture, fract(uB), dFdx(uB), dFdy(uB));
    vec4 t3 = textureGrad(textTexture, fract(uC), dFdx(uC), dFdy(uC));

    float hueBase = fract(t * 0.06);
    float sat = 0.7 + 0.25 * sin(t * 0.4 + sr);
    float val = 0.6 + 0.35 * sin(t * 0.5 + dot(TexCoord, vec2(2.3, 1.9)));
    vec3 tint = hsv2rgb(vec3(hueBase + r * 0.25, sat, val));

    float slowBeat = 0.5 + 0.5 * sin(t * 0.8 + aMix * 0.05);
    float mix1 = 0.5 + 0.5 * sin(t * 0.6);
    float mix2 = 0.5 + 0.5 * cos(t * 0.45);

    vec3 warpCol = mix(sin(t1.rgb * pingPong(time_f * PI, 10.0)), sin(t2.rgb * pingPong(time_f * PI / 2.0, 8.0)), mix1);
    warpCol = mix(warpCol, t3.rgb, mix2 * 0.5);
    warpCol *= tint * (0.92 + 0.08 * slowBeat);

    vec2 dtx = dFdx(TexCoord);
    vec2 dty = dFdy(TexCoord);

    vec3 base = textureGrad(textTexture, TexCoord, dtx, dty).rgb;

    vec4 s1 = textureGrad(textTexture, TexCoord, dtx, dty);
    vec4 s2 = textureGrad(textTexture, TexCoord * 0.5, dtx * 0.5, dty * 0.5);
    vec4 s3 = textureGrad(textTexture, TexCoord * 0.25, dtx * 0.25, dty * 0.25);
    vec4 s4 = textureGrad(textTexture, TexCoord * 0.125, dtx * 0.125, dty * 0.125);
    vec3 multi = (s1.rgb + s2.rgb + s3.rgb + s4.rgb) * 0.25;

    vec3 combined = mix(base, warpCol * 3.0, 0.6);

    float multiMix = 0.45 + 0.35 * sin(t * 0.25 + aMix * 0.02);
    vec3 multiBlend = mix(base, multi, 0.7);
    combined = mix(combined, multiBlend, multiMix);

    vec3 bloom = combined * combined * 0.18 + pow(max(combined - 0.6, 0.0), vec3(2.0)) * 0.10;
    combined += bloom;

    combined = clamp(combined, vec3(0.0), vec3(1.0));
    color = vec4(combined, 1.0);
}
)SHD";
inline const char *src_frac_shader04_echo3_spin = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length){
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect, float spin){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x) + spin;
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i = 0; i < 6; i++){
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t){
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.05);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

void main(void){
    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);
    vec2 m = (iMouse.z > 0.5 ? (iMouse.xy / iResolution) : vec2(0.5));

    float t = time_f + iTime;
    float sr = clamp(iSampleRate / 48000.0, 0.25, 4.0);
    float aMix = clamp(amp * 0.7 + uamp * 0.3, 0.0, 20.0);

    float seg = 4.0 + 2.0 * sin(t * 0.25);
    float zoom = 1.45 + 0.45 * sin(t * 0.35 + 0.2 * sr);
    float kaleidoSpin = t * 0.5 + aMix * 0.05;

    vec2 kUV = reflectUV(TexCoord, seg, m, ar.x, kaleidoSpin);
    kUV = fractalFold(kUV, zoom, t, m, ar.x);

    float radial = length((kUV - m) * ar);
    float globalSpin = t * 0.6;
    kUV = rotateUV(kUV, globalSpin + radial * 4.0, m, ar.x);

    vec2 dir = (kUV - m) * ar;
    float r = length(dir);
    float ripple = sin(20.0 * r - t * 9.0) * 0.012 / (1.0 + 18.0 * r);
    ripple *= 1.0 / (1.0 + 12.0 * (abs(dFdx(r)) + abs(dFdy(r))));

    vec2 uA = kUV + normalize(dir + 1e-5) * ripple;
    vec2 uB = mix(TexCoord, kUV, 0.88);
    vec2 uC = mix(TexCoord, kUV, 0.94) + vec2(0.002 * sin(t), 0.002 * cos(t));

    vec4 t1 = textureGrad(textTexture, fract(uA), dFdx(uA), dFdy(uA));
    vec4 t2 = textureGrad(textTexture, fract(uB), dFdx(uB), dFdy(uB));
    vec4 t3 = textureGrad(textTexture, fract(uC), dFdx(uC), dFdy(uC));

    float hueBase = fract(t * 0.06);
    float sat = 0.7 + 0.25 * sin(t * 0.4 + sr);
    float val = 0.6 + 0.35 * sin(t * 0.5 + dot(TexCoord, vec2(2.3, 1.9)));

    vec3 tint = hsv2rgb(vec3(hueBase + r * 0.25, sat, val));

    float mix1 = 0.5 + 0.5 * sin(t * 0.6);
    float mix2 = 0.5 + 0.5 * cos(t * 0.45);

    vec3 warpCol = mix(sin(t1.rgb * pingPong(time_f * PI, 10.0)), sin(t2.rgb * pingPong(time_f * PI / 2.0, 8.0)), mix1);
    warpCol = mix(warpCol, t3.rgb, mix2 * 0.5);
    warpCol *= tint;

    vec2 dtx = dFdx(TexCoord);
    vec2 dty = dFdy(TexCoord);

    vec3 base = textureGrad(textTexture, TexCoord, dtx, dty).rgb;

    vec4 s1 = textureGrad(textTexture, TexCoord, dtx, dty);
    vec4 s2 = textureGrad(textTexture, TexCoord * 0.5, dtx * 0.5, dty * 0.5);
    vec4 s3 = textureGrad(textTexture, TexCoord * 0.25, dtx * 0.25, dty * 0.25);
    vec4 s4 = textureGrad(textTexture, TexCoord * 0.125, dtx * 0.125, dty * 0.125);
    vec3 multi = (s1.rgb + s2.rgb + s3.rgb + s4.rgb) * 0.25;

    vec3 multiBlend = mix(base, multi, 0.7);

    vec3 pattern = mix(warpCol, multiBlend, 0.4);

    float PATTERN_ALPHA = 0.85;
    float BASE_ALPHA = 0.15;

    vec3 combined = base * BASE_ALPHA + pattern * PATTERN_ALPHA;

    vec3 bloom = combined * combined * 0.18 + pow(max(combined - 0.6, 0.0), vec3(2.0)) * 0.10;
    combined += bloom;

    combined = clamp(combined, vec3(0.0), vec3(1.0));
    color = vec4(combined, 1.0);
}
)SHD";
inline const char *src_frac_shader04_grid = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
float pingPong(float x, float length){
    float m = mod(x, length*2.0);
    return m <= length ? m : length*2.0 - m;
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i=0;i<6;i++){
        p = abs((p - c) * (zoom + 0.15*sin(t*0.35+float(i)))) - 0.5 + c;
        p = rotateUV(p, t*0.12 + float(i)*0.07, c, aspect);
    }
    return p;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 movingGradient(vec2 uv, vec2 c, float t, float aspect){
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    vec2 d = normalize(vec2(cos(t*0.27), sin(t*0.31)));
    float s = dot(p, d);
    float band = 0.5 + 0.5*sin(s*6.28318530718*0.35 + t*0.9);
    float h = fract(s*0.22 + t*0.07 + 0.15*sin(t*0.33));
    float S = 0.75 + 0.25*sin(t*0.21 + s*2.0);
    float V = 0.75 + 0.25*band;
    vec3 base = hsv2rgb(vec3(h, S, V));
    float edge = smoothstep(0.2, 0.8, band);
    return mix(base*0.6, base, edge);
}

void main(){
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = TexCoord;

    vec4 originalTexture = texture(textTexture, TexCoord);

    float seg = 6.0 + 2.0*sin(time_f*0.33);
    vec2 kUV = reflectUV(uv, seg, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);

    vec2 p = (kUV - m) * ar;
    float base = 1.82 + 0.18*sin(time_f*0.2);
    float period = log(base);
    float tz = time_f * 0.65;
    float r = length(p) + 1e-6;
    float ang = atan(p.y, p.x) + tz * 0.35 + 0.35*sin(r*9.0 + time_f*0.8);
    float k = fract((log(r) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap*1.045) / ar + m);
    vec2 u2 = fract((pwrap*0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f*1.3)) * vec2(1.0, 1.0/aspect);

    float hue = fract(ang*0.15 + time_f*0.08 + k*0.5);
    float sat = 0.8 - 0.2*cos(time_f*0.7 + r*6.0);
    float val = 0.8 + 0.2*sin(time_f*0.9 + k*6.28318530718);
    vec3 tint = hsv2rgb(vec3(hue, sat, val));

    float ring = smoothstep(0.0, 0.7, sin(log(r+1e-3)*8.0 + time_f*1.2));
    float pulse = 0.5 + 0.5*sin(time_f*2.0 + r*18.0 + k*12.0);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m)*ar));
    vign = mix(0.85, 1.2, vign);

    float blendFactor = 0.58;

    float rC = texture(textTexture, u0 + off).r;
    float gC = texture(textTexture, u1).g;
    float bC = texture(textTexture, u2 - off).b;
    vec3 kaleidoRGB = vec3(rC, gC, bC);

    vec4 kaleidoColor = vec4(kaleidoRGB, 1.0) * vec4(tint, 1.0);
    vec4 merged = mix(kaleidoColor, originalTexture, blendFactor);

    merged.rgb *= (0.75 + 0.25*ring) * (0.85 + 0.15*pulse) * vign;

    vec3 bloom = merged.rgb * merged.rgb * 0.18 + pow(max(merged.rgb-0.6,0.0), vec3(2.0))*0.12;
    vec3 outFractal = merged.rgb + bloom;

    float wob = 0.9 + 0.1*sin(time_f + r*12.0 + k*9.0);
    outFractal *= wob;

    vec3 grad = movingGradient(TexCoord, m, time_f, aspect);
    float gradAmt = 0.35 + 0.25*sin(time_f*0.5 + r*5.0 + k*9.0);
    vec3 screenBlend = 1.0 - (1.0 - outFractal) * (1.0 - grad);
    outFractal = mix(outFractal, screenBlend, gradAmt);

    outFractal = mix(outFractal, outFractal * originalTexture.rgb, 0.8);

    float sparkle = abs(sin(time_f * 10.0 + TexCoord.x * 100.0) * cos(time_f * 15.0 + TexCoord.y * 100.0));

    vec3 gridMixed = mix(originalTexture.rgb, outFractal, sparkle);

    vec3 outCol = sin(gridMixed * (0.5 + 0.5*pingPong(time_f, 12.0)));
    outCol = clamp(outCol, vec3(0.08), vec3(0.96));

    color = vec4(outCol, 1.0);
}
)SHD";
inline const char *src_frac_shader04_julia = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
float pingPong(float x, float length){
    float m = mod(x, length*2.0);
    return m <= length ? m : length*2.0 - m;
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 movingGradient(vec2 uv, vec2 c, float t, float aspect){
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    vec2 d = normalize(vec2(cos(t*0.27), sin(t*0.31)));
    float s = dot(p, d);
    float band = 0.5 + 0.5*sin(s*6.28318530718*0.35 + t*0.9);
    float h = fract(s*0.22 + t*0.07 + 0.15*sin(t*0.33));
    float S = 0.75 + 0.25*sin(t*0.21 + s*2.0);
    float V = 0.75 + 0.25*band;
    vec3 base = hsv2rgb(vec3(h, S, V));
    float edge = smoothstep(0.2, 0.8, band);
    return mix(base*0.6, base, edge);
}

mat2 rot2(float a){ float s=sin(a), c=cos(a); return mat2(c,-s,s,c); }

void juliaEval(vec2 z0, vec2 cJ, out float iterCount, out float smoothIter, out float orbitTrapVal){
    vec2 z = z0;
    float i = 0.0;
    orbitTrapVal = 1e9;
    const int MAX_IT = 120;
    for(int k=0;k<MAX_IT;k++){
        orbitTrapVal = min(orbitTrapVal, length(z));
        vec2 z2 = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + cJ;
        z = z2;
        i += 1.0;
        if(dot(z,z) > 256.0) break;
    }
    float rl = length(z);
    smoothIter = i - log2(max(0.000001, log(max(rl, 1e-6))));
    iterCount = i;
}

float juliaScore(vec2 z0, vec2 cJ){
    vec2 z = z0;
    const int MAX_S = 28;
    float i = 0.0;
    for(int k=0;k<MAX_S;k++){
        vec2 z2 = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + cJ;
        z = z2;
        i += 1.0;
        if(dot(z,z) > 256.0) break;
    }
    float rl = length(z);
    float sI = i - log2(max(0.000001, log(max(rl, 1e-6))));
    float tgt = 18.0;
    float score = 1.0/(1.0 + abs(sI - tgt));
    return score;
}

vec2 findAnchor(vec2 base, vec2 cJ, float rad){
    float best = -1.0;
    vec2 bestOff = vec2(0.0);
    for(int k=0;k<8;k++){
        float a = (6.28318530718/8.0)*float(k) + time_f*0.11;
        vec2 off = vec2(cos(a), sin(a)) * rad;
        float s = juliaScore(base + off, cJ);
        if(s > best){ best = s; bestOff = off; }
    }
    return bestOff;
}

void main(){
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = TexCoord;
    vec4 originalTexture = texture(textTexture, TexCoord);

    float t = time_f;
    float zoomSpeed = 0.28;
    float spin = 0.23;
    float scale = exp(-t * zoomSpeed);
    vec2 p = (uv - m) * ar;
    p = rot2(t*spin) * p;

    vec2 cJ = (iMouse.z > 0.5)
        ? ((iMouse.xy/iResolution)*2.0-1.0) * vec2(aspect,1.0) * 0.6
        : vec2(0.285 + 0.15*sin(t*0.17), 0.01 + 0.15*cos(t*0.21));

    float probeRad = 0.75 * scale;
    vec2 anchor = findAnchor(vec2(0.0), cJ, probeRad);
    vec2 z0 = (p + anchor) * scale * 2.0;

    float iterCount, smoothIter, orbitTrapVal;
    juliaEval(z0, cJ, iterCount, smoothIter, orbitTrapVal);

    float rD = length(p);
    float hue = fract(0.12*smoothIter + 0.07*t + 0.15*sin(orbitTrapVal*3.0));
    float sat = 0.65 + 0.25*sin(0.7*t + orbitTrapVal*5.0);
    float val = 0.80 + 0.20*sin(0.9*t + smoothIter*0.35);
    vec3 tint = hsv2rgb(vec3(hue, sat, val));

    float ring = smoothstep(0.0, 0.7, sin(log(rD*scale+1e-3)*9.5 + t*1.2));
    float pulse = 0.5 + 0.5*sin(t*2.0 + rD*28.0 + smoothIter*0.6);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m)*ar));
    vign = mix(0.85, 1.2, vign);

    vec2 pr = z0;
    vec2 dir = normalize(pr + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(t*1.3)) * vec2(1.0, 1.0/aspect);

    vec2 u0 = fract(pr/ar + m);
    vec2 u1 = fract((pr*1.045)/ar + m);
    vec2 u2 = fract((pr*0.955)/ar + m);

    float rC = texture(textTexture, u0 + off).r;
    float gC = texture(textTexture, u1).g;
    float bC = texture(textTexture, u2 - off).b;
    vec3 texRGB = vec3(rC, gC, bC);

    float shade = smoothstep(0.0, 1.0, 1.0 - clamp(orbitTrapVal*0.5, 0.0, 1.0));
    vec3 fractRGB = mix(texRGB, texRGB*tint, 0.65) * mix(0.7, 1.25, shade);
    vec4 fractalColor = vec4(fractRGB * tint, 1.0);

    float blendFactor = 0.58;
    vec4 merged = mix(fractalColor, originalTexture, blendFactor);
    merged.rgb *= (0.75 + 0.25*ring) * (0.85 + 0.15*pulse) * vign;

    vec3 bloom = merged.rgb * merged.rgb * 0.18 + pow(max(merged.rgb-0.6,0.0), vec3(2.0))*0.12;
    vec3 outCol = merged.rgb + bloom;

    float wob = 0.9 + 0.1*sin(t + rD*14.0 + smoothIter*0.5);
    outCol *= wob;

    vec3 grad = movingGradient(TexCoord, m, t, aspect);
    float gradAmt = 0.35 + 0.25*sin(t*0.5 + rD*7.0 + smoothIter*0.35);
    vec3 screenBlend = 1.0 - (1.0 - outCol) * (1.0 - grad);
    outCol = mix(outCol, screenBlend, gradAmt);

    vec4 tcol = texture(textTexture, TexCoord);
    outCol = mix(outCol, outCol*tcol.rgb, 0.8);

    outCol = sin(outCol * (0.5 + 0.5*pingPong(t, 12.0)));
    outCol = clamp(outCol, vec3(0.08), vec3(0.96));

    color = vec4(outCol, 1.0);
}
)SHD";
inline const char *src_frac_shader05 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
float pingPong(float x, float length){
    float m = mod(x, length*2.0);
    return m <= length ? m : length*2.0 - m;
}

vec3 hsv2rgb(vec3 c){
    vec4 K=vec4(1.0,2.0/3.0,1.0/3.0,3.0);
    vec3 p=abs(fract(c.xxx+K.xyz)*6.0-K.www);
    return c.z*mix(K.xxx,clamp(p-K.xxx,0.0,1.0),c.y);
}

vec3 movingGradient(vec2 uv, vec2 c, float t, float aspect){
    vec2 ar=vec2(aspect,1.0);
    vec2 p=(uv-c)*ar;
    vec2 d=normalize(vec2(cos(t*0.27),sin(t*0.31)));
    float s=dot(p,d);
    float band=0.5+0.5*sin(s*6.28318530718*0.35+t*0.9);
    float h=fract(s*0.22+t*0.07+0.15*sin(t*0.33));
    float S=0.75+0.25*sin(t*0.21+s*2.0);
    float V=0.75+0.25*band;
    vec3 base=hsv2rgb(vec3(h,S,V));
    float edge=smoothstep(0.2,0.8,band);
    return mix(base*0.6,base,edge);
}

mat2 rot2(float a){float s=sin(a),c=cos(a);return mat2(c,-s,s,c);}

void main(){
    float aspect=iResolution.x/iResolution.y;
    vec2 ar=vec2(aspect,1.0);
    vec2 m=(iMouse.z>0.5)?(iMouse.xy/iResolution):vec2(0.5);
    vec2 p=(TexCoord-m)*ar;

    float T=8.0;
    float s=pingPong(time_f,T)/T;
    float e=s*s*(3.0-2.0*s);
    float z=min(1.0,mix(0.55,2.8,e));
    float spin=0.35*(e-0.5);
    p=rot2(spin)*p;
    vec2 uvTex=p/z/ar+m;

    vec4 baseTex=texture(textTexture,uvTex);
    vec3 grad=movingGradient(TexCoord,m,time_f,aspect);

    float vign=1.0-smoothstep(0.78,1.15,length((TexCoord-m)*ar));
    vign=mix(0.86,1.18,vign);

    float chroma=0.0025*(0.2+abs(e-0.5)*1.8);
    vec3 texRGB;
    texRGB.r=texture(textTexture,uvTex+vec2(chroma,0.0)).r;
    texRGB.g=texture(textTexture,uvTex).g;
    texRGB.b=texture(textTexture,uvTex-vec2(chroma,0.0)).b;

    float growMask=smoothstep(0.0,0.6,e)*smoothstep(1.0,0.6,e);
    float pulse=0.5+0.5*sin(time_f*2.0+length(p)*24.0);
    float mixTex=0.55+0.35*growMask;
    float mixGrad=0.35+0.25*(1.0-growMask);

    vec3 screenBlend=1.0-(1.0-texRGB)*(1.0-grad);
    vec3 col=mix(texRGB,screenBlend,mixGrad);
    col=mix(col,baseTex.rgb,mixTex);
    col*=vign*(0.85+0.15*pulse);

    vec3 bloom=col*col*0.18+pow(max(col-0.6,0.0),vec3(2.0))*0.12;
    col+=bloom;

    col=sin(col*(0.5+0.5*pingPong(time_f,12.0)));
    col=clamp(col,vec3(0.08),vec3(0.96));

    color=vec4(col,1.0);
}
)SHD";
inline const char *src_frac_star1 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float hash1(vec2 p){ return fract(sin(dot(p, vec2(127.1,311.7))) * 43758.5453); }
vec2 hash2(vec2 p){ return fract(sin(vec2(dot(p, vec2(127.1,311.7)), dot(p, vec2(269.5,183.3)))) * 43758.5453); }
float pingPong(float x,float l){ float m=mod(x,l*2.0); return m<=l?m:l*2.0-m; }

vec3 hsv2rgb(vec3 c){
    vec4 K=vec4(1.0,2.0/3.0,1.0/3.0,3.0);
    vec3 p=abs(fract(c.xxx+K.xyz)*6.0-K.www);
    return c.z*mix(K.xxx,clamp(p-K.xxx,0.0,1.0),c.y);
}
vec3 neonPalette(float t){
    vec3 a=vec3(1.0,0.15,0.75), b=vec3(0.10,0.55,1.0), c=vec3(0.10,1.0,0.45);
    float ph=fract(t*0.08);
    vec3 k1=mix(a,b,smoothstep(0.00,0.33,ph));
    vec3 k2=mix(b,c,smoothstep(0.33,0.66,ph));
    vec3 k3=mix(c,a,smoothstep(0.66,1.00,ph));
    float A=step(ph,0.33), B=step(0.33,ph)*step(ph,0.66), C=step(0.66,ph);
    return normalize(A*k1+B*k2+C*k3)*1.05;
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res){
    vec2 ts=1.0/res;
    vec3 s00=textureGrad(img, uv+ts*vec2(-1,-1), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10=textureGrad(img, uv+ts*vec2( 0,-1), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20=textureGrad(img, uv+ts*vec2( 1,-1), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01=textureGrad(img, uv+ts*vec2(-1, 0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11=textureGrad(img, uv,                 dFdx(uv), dFdy(uv)).rgb;
    vec3 s21=textureGrad(img, uv+ts*vec2( 1, 0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02=textureGrad(img, uv+ts*vec2(-1, 1), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12=textureGrad(img, uv+ts*vec2( 0, 1), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22=textureGrad(img, uv+ts*vec2( 1, 1), dFdx(uv), dFdy(uv)).rgb;
    return (s00+2.0*s10+s20+2.0*s01+4.0*s11+2.0*s21+s02+2.0*s12+s22)/16.0;
}
vec3 softTone(vec3 c){
    c=pow(max(c,0.0), vec3(0.95));
    float l=dot(c, vec3(0.299,0.587,0.114));
    c=mix(vec3(l), c, 0.9);
    return clamp(c,0.0,1.0);
}

vec2 sitePos(vec2 id, float grid){
    vec2 j=(hash2(id)*2.0-1.0)*(0.33+0.10*sin(time_f*0.37+6.2831*hash1(id+3.71)));
    return (id+0.5+j)/grid;
}

float starMetric(vec2 d, float n, float amp){
    float ang=atan(d.y,d.x);
    float r=length(d);
    float mod=1.0+amp*cos(n*ang);
    return r/max(0.001,mod);
}

void main(){
    vec2 res=iResolution;
    float aspect=res.x/res.y;
    vec2 focus=(iMouse.z>0.5)?(iMouse.xy/res):vec2(0.5);

    float t=pingPong(time_f*0.35,1.0);
    float zoom=mix(0.7,2.6,t);
    vec2 uvZ=(TexCoord-focus)*zoom+focus;

    float grid=mix(18.0,28.0,0.5+0.5*sin(time_f*0.11));

    vec2 gtc=uvZ*grid;
    vec2 gid0=floor(gtc);

    float best1=1e9,best2=1e9;
    vec2 bestId=vec2(0.0), bestSite=vec2(0.0);
    float bestN=7.0, bestA=0.3;

    for(int j=-1;j<=1;j++){
        for(int i=-1;i<=1;i++){
            vec2 nid=gid0+vec2(i,j);
            vec2 s=sitePos(nid,grid);
            vec2 d=uvZ-s; d.x*=aspect;
            float nPts=floor(7.0+5.0*hash1(nid+7.7));
            float amp=0.28+0.22*hash1(nid+4.2);
            float dist=starMetric(d,nPts,amp);
            if(dist<best1){
                best2=best1;
                best1=dist;
                bestId=nid;
                bestSite=s;
                bestN=nPts;
                bestA=amp;
            }else if(dist<best2){
                best2=dist;
            }
        }
    }

    float edge=0.5*(best2-best1);
    float px=1.0/min(res.x,res.y);
    float border=1.0-smoothstep(px*0.75, px*0.75+fwidth(edge), edge);

    vec2 rel=uvZ-bestSite; rel.x*=aspect;

    float rot=6.2831853*hash1(bestId+11.9)+time_f*(0.10+0.08*hash1(bestId+2.2));
    float cs=cos(rot), sn=sin(rot);
    vec2 rrel=mat2(cs,-sn,sn,cs)*rel;

    vec2 pieceUV=rrel; pieceUV.x/=aspect; pieceUV+=bestSite;

    vec4 texCol=texture(textTexture,pieceUV);

    float ang=atan(rrel.y,rrel.x);
    float flare=pow(0.5+0.5*cos(bestN*ang+time_f*0.7+6.2831*hash1(bestId+3.4)),3.0);
    vec3 glow=hsv2rgb(vec3(fract(hash1(bestId+9.3)+time_f*0.05),0.85,1.0));
    vec3 neon=neonPalette(time_f+hash1(bestId+1.1)*8.0);
    vec3 tint=mix(neon,glow,0.45)*(0.55+0.45*flare);
    float rad=length(rrel);
    float vign=smoothstep(0.9,0.2,rad/(0.55+0.05*sin(time_f*0.5+hash1(bestId+5.5))));
    vec3 pieceCol=mix(texCol.rgb, texCol.rgb*tint, 0.25*vign);

    vec3 pre=tentBlur3(textTexture, TexCoord, res);
    vec3 inside=mix(pre, pieceCol, 0.85);

    vec3 borderCol=mix(vec3(0.02,0.02,0.03), neonPalette(time_f*0.5), 0.15);
    vec3 outRGB=mix(inside, sin(borderCol *  (time_f * PI)), border);
    outRGB=softTone(outRGB);

    color=vec4(outRGB, texCol.a);
}
)SHD";
inline const char *src_frac_zoom1 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    vec2 pa = abs(p);
    vec2 ps = sin(abs(p));
    float mixAmt = 0.5 + 0.5 * sin(time_f * 0.2);
    vec2 pm = mix(pa, ps, mixAmt);
    return max(pm.x, pm.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    float aspect = iResolution.x / iResolution.y;
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);

    vec2 uv = TexCoord * 2.0 - 1.0;
    uv.x *= aspect;
    float rLen = length(uv);
    float radialWave = pingPong(sin(rLen * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, radialWave);

    vec3 baseCol = preBlendColor(TexCoord);

    float audioLevel = clamp(amp * 0.75 + uamp * 0.40, 0.0, 2.0);
    float tZoom = pingPong(time_f * 0.12 + audioLevel * 0.35, 1.0);
    float zoomBase = mix(0.90, 2.40, tZoom);
    float zoomAudio = 1.0 + audioLevel * 0.75;
    float foldZoom = zoomBase * zoomAudio;

    float seg = 4.0 + 2.0 * sin(time_f * 0.33 + audioLevel * 0.25);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * (0.23 + audioLevel * 0.05), m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0 + audioLevel);
    float tz = time_f * 0.65 + audioLevel * 0.35;

    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6 + audioLevel * 3.0);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);

    float zoomShell = 1.0 + audioLevel * 0.8;
    vec2 pwrap = vec2(cos(ang), sin(ang)) * (rw * zoomShell);

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3 + audioLevel)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2 + audioLevel * 2.0));
    ring *= pingPong(time_f * PI, 5.0);

    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0 + audioLevel * 6.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;

    outCol = mix(outCol, baseCol, 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));

    float mixGlow = pingPong(glow * PI, 5.0) * (0.4 + 0.4 * clamp(audioLevel, 0.0, 1.0));
    vec3 finalRGB = mix(baseTex.rgb, outCol, mixGlow);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_zoom2 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    vec2 pa = abs(p);
    vec2 ps = sin(abs(p));
    float mixAmt = 0.5 + 0.5 * sin(time_f * 0.2);
    vec2 pm = mix(pa, ps, mixAmt);
    return max(pm.x, pm.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 triUV(vec2 uv, float scale) {
    vec2 p = uv * scale;
    vec2 g = floor(p);
    vec2 f = fract(p);
    if (f.x + f.y > 1.0) {
        g += 1.0;
        f = 1.0 - f.yx;
    }
    return (g + f) / scale;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    float aspect = iResolution.x / iResolution.y;
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);

    vec2 uv = TexCoord * 2.0 - 1.0;
    uv.x *= aspect;
    float rLen = length(uv);
    float radialWave = pingPong(sin(rLen * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, radialWave);

    vec3 baseCol = preBlendColor(TexCoord);

    float audioLevel = clamp(amp * 0.75 + uamp * 0.40, 0.0, 2.0);
    float tZoom = pingPong(time_f * 0.12 + audioLevel * 0.35, 1.0);
    float zoomBase = mix(0.95, 1.85, tZoom);
    float zoomAudio = 1.0 + audioLevel * 0.45;
    float foldZoom = zoomBase * zoomAudio;

    float seg = 4.0 + 2.0 * sin(time_f * 0.33 + audioLevel * 0.25);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * (0.23 + audioLevel * 0.05), m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0 + audioLevel);
    float tz = time_f * 0.65 + audioLevel * 0.35;

    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6 + audioLevel * 3.0);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    rw = clamp(rw, 0.45, 2.5);

    float zoomShell = 1.0 + audioLevel * 0.5;
    vec2 pwrap = vec2(cos(ang), sin(ang)) * (rw * zoomShell);

    float triScale = 80.0 * (1.0 + 0.3 * audioLevel);
    vec2 baseTri = triUV(fract(pwrap / ar + m), triScale);
    vec2 tri0 = baseTri;
    vec2 tri1 = triUV(fract(pwrap * 1.045 / ar + m), triScale);
    vec2 tri2 = triUV(fract(pwrap * 0.955 / ar + m), triScale);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3 + audioLevel)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(tri0 + off);
    vec3 gC = preBlendColor(tri1);
    vec3 bC = preBlendColor(tri2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2 + audioLevel * 2.0));
    ring *= pingPong(time_f * PI, 5.0);

    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0 + audioLevel * 6.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;

    outCol = mix(outCol, baseCol, 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));

    float mixGlow = pingPong(glow * PI, 5.0) * (0.4 + 0.4 * clamp(audioLevel, 0.0, 1.0));
    vec3 finalRGB = mix(baseTex.rgb, outCol, mixGlow);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_zoom3 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    vec2 pa = abs(p);
    vec2 ps = sin(abs(p));
    float mixAmt = 0.5 + 0.5 * sin(time_f * 0.2);
    vec2 pm = mix(pa, ps, mixAmt);
    return max(pm.x, pm.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 triUV(vec2 uv, float scale) {
    vec2 p = uv * scale;
    vec2 g = floor(p);
    vec2 f = fract(p);
    if (f.x + f.y > 1.0) {
        g += 1.0;
        f = 1.0 - f.yx;
    }
    return (g + f) / scale;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    float aspect = iResolution.x / iResolution.y;
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);

    vec2 uv = TexCoord * 2.0 - 1.0;
    uv.x *= aspect;
    float rLen = length(uv);
    float radialWave = pingPong(sin(rLen * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, radialWave);

    vec3 baseCol = preBlendColor(TexCoord);

    float audioLevel = clamp(amp * 0.75 + uamp * 0.40, 0.0, 2.0);
    float tZoom = pingPong(time_f * 0.12 + audioLevel * 0.35, 1.0);
    float zoomBase = mix(0.98, 1.35, tZoom);
    float zoomAudio = 1.0 + audioLevel * 0.30;
    float foldZoom = zoomBase * zoomAudio;

    float seg = 4.0 + 2.0 * sin(time_f * 0.33 + audioLevel * 0.25);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * (0.23 + audioLevel * 0.05), m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0 + audioLevel);
    float tz = time_f * 0.65 + audioLevel * 0.35;

    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6 + audioLevel * 3.0);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    rw = clamp(rw, 0.7, 1.6);

    float zoomShell = 1.0 + audioLevel * 0.35;
    vec2 pwrap = vec2(cos(ang), sin(ang)) * (rw * zoomShell * 0.85);

    float triScale = 24.0 * (1.0 + 0.15 * audioLevel);
    vec2 baseTri = triUV(fract(pwrap / ar + m), triScale);
    vec2 tri0 = baseTri;
    vec2 tri1 = triUV(fract(pwrap * 1.045 / ar + m), triScale);
    vec2 tri2 = triUV(fract(pwrap * 0.955 / ar + m), triScale);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3 + audioLevel)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(tri0 + off);
    vec3 gC = preBlendColor(tri1);
    vec3 bC = preBlendColor(tri2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2 + audioLevel * 2.0));
    ring *= pingPong(time_f * PI, 5.0);

    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0 + audioLevel * 6.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;

    outCol = mix(outCol, baseCol, 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));

    float mixGlow = pingPong(glow * PI, 5.0) * (0.4 + 0.4 * clamp(audioLevel, 0.0, 1.0));
    vec3 finalRGB = mix(baseTex.rgb, outCol, mixGlow);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_zoom4 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 complexIter(vec2 z, vec2 c) {
    return vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
}

vec2 fractalZoomUV(vec2 uv, vec2 center, float aspect, float t) {
    vec2 p = (uv - center) * vec2(aspect, 1.0);

    float tZoom = t * 0.23;
    float level = floor(tZoom);
    float local = pingPong(fract(tZoom), 1.0);

    float zoom = pow(2.15, level + local * 0.98);
    zoom = min(zoom, 50000.0);

    p *= zoom;

    vec2 z = p * 0.42;
    vec2 c = vec2(0.32 + 0.02 * sin(t * 0.17),
                  0.043 + 0.015 * cos(t * 0.21));

    for (int i = 0; i < 7; i++) {
        z = complexIter(z, c);
    }

    float warp = 0.30 + 0.15 * sin(t * 0.5);
    float lenZ = max(1.0, length(z));
    p += warp * (z / lenZ);

    p /= zoom;
    p.x /= aspect;
    return p + center;
}
void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 baseUV0 = pwrap / ar + m;
    vec2 baseUV1 = (pwrap * 1.045) / ar + m;
    vec2 baseUV2 = (pwrap * 0.955) / ar + m;

    vec2 u0 = fract(fractalZoomUV(baseUV0, m, aspect, time_f));
    vec2 u1 = fract(fractalZoomUV(baseUV1, m, aspect, time_f + 0.7));
    vec2 u2 = fract(fractalZoomUV(baseUV2, m, aspect, time_f + 1.4));

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;

    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));

    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_zoom5 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 complexIter(vec2 z, vec2 c) {
    return vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
}

vec2 fractalZoomUV(vec2 uv, vec2 center, float aspect, float t) {
    vec2 p = (uv - center) * vec2(aspect, 1.0);
    float tZoom = t * 0.23;
    float level = floor(tZoom);
    float local = pingPong(fract(tZoom), 1.0);
    float zoom = pow(2.15, level + local * 0.98);
    zoom = min(zoom, 50000.0);
    p *= zoom;
    vec2 z = p * 0.42;
    vec2 c = vec2(0.32 + 0.02 * sin(t * 0.17),
                  0.043 + 0.015 * cos(t * 0.21));
    for (int i = 0; i < 7; i++) {
        z = complexIter(z, c);
    }
    float warp = 0.30 + 0.15 * sin(t * 0.5);
    float lenZ = max(1.0, length(z));
    p += warp * (z / lenZ);
    float ang = t * 0.45 * 6.28318530718;
    float cs = cos(ang);
    float sn = sin(ang);
    p = mat2(cs, -sn, sn, cs) * p;
    p /= zoom;
    p.x /= aspect;
    return p + center;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 baseUV0 = pwrap / ar + m;
    vec2 baseUV1 = (pwrap * 1.045) / ar + m;
    vec2 baseUV2 = (pwrap * 0.955) / ar + m;
    vec2 u0 = fract(fractalZoomUV(baseUV0, m, aspect, time_f));
    vec2 u1 = fract(fractalZoomUV(baseUV1, m, aspect, time_f + 0.7));
    vec2 u2 = fract(fractalZoomUV(baseUV2, m, aspect, time_f + 1.4));
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_zoom6 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.10, 0.85);
    vec3 blue = vec3(0.00, 0.60, 1.0);
    vec3 green = vec3(0.05, 1.05, 0.50);
    float ph = fract(t * 0.15);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    vec3 col = a * k1 + b * k2 + c * k3;
    col = pow(col, vec3(0.85));
    return normalize(col) * 1.4;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.9));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.7);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 complexIter(vec2 z, vec2 c) {
    return vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
}

vec2 fractalZoomUV(vec2 uv, vec2 center, float aspect, float t) {
    vec2 p = (uv - center) * vec2(aspect, 1.0);
    float tZoom = t * 0.23;
    float level = floor(tZoom);
    float local = pingPong(fract(tZoom), 1.0);
    float zoom = pow(2.15, level + local * 0.98);
    zoom = min(zoom, 50000.0);
    p *= zoom;
    vec2 z = p * 0.42;
    vec2 c = vec2(0.32 + 0.02 * sin(t * 0.17),
                  0.043 + 0.015 * cos(t * 0.21));
    for (int i = 0; i < 7; i++) {
        z = complexIter(z, c);
    }
    float warp = 0.30 + 0.15 * sin(t * 0.5);
    float lenZ = max(1.0, length(z));
    p += warp * (z / lenZ);
    float ang = t * 0.45 * 6.28318530718;
    float cs = cos(ang);
    float sn = sin(ang);
    p = mat2(cs, -sn, sn, cs) * p;
    p /= zoom;
    p.x /= aspect;
    return p + center;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 baseUV0 = pwrap / ar + m;
    vec2 baseUV1 = (pwrap * 1.045) / ar + m;
    vec2 baseUV2 = (pwrap * 0.955) / ar + m;
    vec2 u0 = fract(fractalZoomUV(baseUV0, m, aspect, time_f));
    vec2 u1 = fract(fractalZoomUV(baseUV1, m, aspect, time_f + 0.7));
    vec2 u2 = fract(fractalZoomUV(baseUV2, m, aspect, time_f + 1.4));
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.18, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.3 + rD * 30.0 + k * 14.0);
    vec3 neonGlobal = neonPalette(time_f + rD * 0.6 + k * 0.8);
    vec3 outCol = kaleidoRGB;
    outCol = mix(outCol, neonGlobal, 0.22 + 0.25 * ring);
    outCol *= (0.78 + 0.28 * ring) * (0.85 + 0.18 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.26 + pow(max(outCol - 0.55, 0.0), vec3(2.2)) * 0.22;
    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.14);
    outCol = clamp(outCol, vec3(0.03), vec3(1.15));
    vec3 finalRGB = mix(baseTex.rgb, outCol, min(0.95, pingPong(glow * PI, 5.0) * 0.95));
    finalRGB = clamp(finalRGB, 0.0, 1.0);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_zoom7 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader01 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
float pingPong(float x, float length){
    float m = mod(x, length*2.0);
    return m <= length ? m : length*2.0 - m;
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i=0;i<6;i++){
        p = abs((p - c) * (zoom + 0.15*sin(t*0.35+float(i)))) - 0.5 + c;
        p = rotateUV(p, t*0.12 + float(i)*0.07, c, aspect);
    }
    return p;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 movingGradient(vec2 uv, vec2 c, float t, float aspect){
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    vec2 d = normalize(vec2(cos(t*0.27), sin(t*0.31)));
    float s = dot(p, d);
    float band = 0.5 + 0.5*sin(s*6.28318530718*0.35 + t*0.9);
    float h = fract(s*0.22 + t*0.07 + 0.15*sin(t*0.33));
    float S = 0.75 + 0.25*sin(t*0.21 + s*2.0);
    float V = 0.75 + 0.25*band;
    vec3 base = hsv2rgb(vec3(h, S, V));
    float edge = smoothstep(0.2, 0.8, band);
    return mix(base*0.6, base, edge);
}

void main(){
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = TexCoord;
    vec4 originalTexture = texture(textTexture, TexCoord);

    float seg = 6.0 + 2.0*sin(time_f*0.33);
    vec2 kUV = reflectUV(uv, seg, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);

    vec2 p = (kUV - m) * ar;
    float base = 1.82 + 0.18*sin(time_f*0.2);
    float period = log(base);
    float tz = time_f * 0.65;
    float r = length(p) + 1e-6;
    float ang = atan(p.y, p.x) + tz * 0.35 + 0.35*sin(r*9.0 + time_f*0.8);
    float k = fract((log(r) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap*1.045) / ar + m);
    vec2 u2 = fract((pwrap*0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f*1.3)) * vec2(1.0, 1.0/aspect);

    float hue = fract(ang*0.15 + time_f*0.08 + k*0.5);
    float sat = 0.8 - 0.2*cos(time_f*0.7 + r*6.0);
    float val = 0.8 + 0.2*sin(time_f*0.9 + k*6.28318530718);
    vec3 tint = hsv2rgb(vec3(hue, sat, val));

    float ring = smoothstep(0.0, 0.7, sin(log(r+1e-3)*8.0 + time_f*1.2));
    float pulse = 0.5 + 0.5*sin(time_f*2.0 + r*18.0 + k*12.0);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m)*ar));
    vign = mix(0.85, 1.2, vign);

    float blendFactor = 0.58;

    float rC = texture(textTexture, u0 + off).r;
    float gC = texture(textTexture, u1).g;
    float bC = texture(textTexture, u2 - off).b;
    vec3 kaleidoRGB = vec3(rC, gC, bC);

    vec4 kaleidoColor = vec4(kaleidoRGB, 1.0) * vec4(tint, 1.0);
    vec4 merged = mix(kaleidoColor, originalTexture, blendFactor);

    merged.rgb *= (0.75 + 0.25*ring) * (0.85 + 0.15*pulse) * vign;

    vec3 bloom = merged.rgb * merged.rgb * 0.18 + pow(max(merged.rgb-0.6,0.0), vec3(2.0))*0.12;
    vec3 outCol = merged.rgb + bloom;

    float wob = 0.9 + 0.1*sin(time_f + r*12.0 + k*9.0);
    outCol *= wob;

    vec3 grad = movingGradient(TexCoord, m, time_f, aspect);
    float gradAmt = 0.35 + 0.25*sin(time_f*0.5 + r*5.0 + k*9.0);
    vec3 screenBlend = 1.0 - (1.0 - outCol) * (1.0 - grad);
    outCol = mix(outCol, screenBlend, gradAmt);

    vec4 t = texture(textTexture, TexCoord);
    outCol = mix(outCol, outCol*t.rgb, 0.8);

    outCol = sin(outCol * (0.5 + 0.5*pingPong(time_f, 12.0)));
    outCol = clamp(outCol, vec3(0.08), vec3(0.96));

    color = vec4(outCol, 1.0);
}
)SHD";
inline const char *src_frac_shader01_dark = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
float pingPong(float x, float length){
    float m = mod(x, length*2.0);
    return m <= length ? m : length*2.0 - m;
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i=0;i<6;i++){
        p = abs((p - c) * (zoom + 0.15*sin(t*0.35+float(i)))) - 0.5 + c;
        p = rotateUV(p, t*0.12 + float(i)*0.07, c, aspect);
    }
    return p;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 movingGradient(vec2 uv, vec2 c, float t, float aspect){
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    vec2 d = normalize(vec2(cos(t*0.27), sin(t*0.31)));
    float s = dot(p, d);
    float band = 0.5 + 0.5*sin(s*6.28318530718*0.35 + t*0.9);
    float h = fract(s*0.22 + t*0.07 + 0.15*sin(t*0.33));
    float S = 0.75 + 0.25*sin(t*0.21 + s*2.0);
    float V = 0.6 + 0.125*band;         // dimmer
    vec3 base = hsv2rgb(vec3(h, S, V));
    float edge = smoothstep(0.2, 0.8, band);
    return mix(base*0.6, base, edge);
}

vec2 triFold(vec2 uv, vec2 c, float aspect){
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    vec2 b1 = vec2(1.0, 0.0);
    vec2 b2 = vec2(0.5, 0.86602540378);
    float u = dot(p, b1);
    float v = dot(p, b2);
    vec2 f = fract(vec2(u, v));
    if(f.x + f.y > 1.0) f = vec2(1.0) - f;
    vec2 q = f.x*b1 + f.y*b2;
    q.x /= aspect;
    return q + c;
}

float diamondRadius(vec2 p){
    p = abs(p);
    return max(p.x, p.y);
}

float luma(vec3 c){ return dot(c, vec3(0.299,0.587,0.114)); }

vec3 bilateral9(vec2 uv, float radiusScale, float sigma_r){
    vec2 texel = 1.0 / iResolution;
    vec2 o = texel * radiusScale;
    vec3 c0 = texture(textTexture, uv).rgb;
    float L0 = luma(c0);
    vec2 offs[9] = vec2[](
        vec2(0,0),
        vec2( o.x, 0), vec2(-o.x, 0),
        vec2(0,  o.y), vec2(0, -o.y),
        vec2( o.x,  o.y), vec2(-o.x,  o.y),
        vec2( o.x, -o.y), vec2(-o.x, -o.y)
    );
    float wsum = 0.0;
    vec3 acc = vec3(0.0);
    for(int i=0;i<9;i++){
        vec3 c = texture(textTexture, uv + offs[i]).rgb;
        float dl = luma(c) - L0;
        float wr = exp(-(dl*dl)/(2.0*sigma_r*sigma_r));
        float dsq = dot(offs[i]/texel, offs[i]/texel);
        float ws = exp(-dsq/2.0);
        float w = wr*ws;
        acc += c*w;
        wsum += w;
    }
    return acc / max(wsum, 1e-6);
}

float localVar9(vec2 uv, float radiusScale){
    vec2 texel = 1.0 / iResolution;
    vec2 o = texel * radiusScale;
    float m = 0.0;
    float s = 0.0;
    int n = 0;
    for(int y=-1;y<=1;y++){
        for(int x=-1;x<=1;x++){
            vec3 c = texture(textTexture, uv + vec2(x,y)*o).rgb;
            float L = luma(c);
            m += L;
            s += L*L;
            n++;
        }
    }
    m /= float(n);
    s /= float(n);
    return max(s - m*m, 0.0);
}

void main(){
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = TexCoord;
    vec4 originalTexture = texture(textTexture, TexCoord);

    float seg = 3.0;
    vec2 kUV = reflectUV(uv, seg, m, aspect);
    kUV = triFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = triFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if(q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18*sin(time_f*0.2);
    float period = log(base);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35*sin(rD*18.0 + time_f*0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap*1.045) / ar + m);
    vec2 u2 = fract((pwrap*0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f*1.3)) * vec2(1.0, 1.0/aspect);

    float hue = fract(ang*0.25 + time_f*0.08 + k*0.5);
    float sat = 0.75 - 0.25*cos(time_f*0.7 + rD*10.0);
    float val = 0.8 + 0.2*sin(time_f*0.9 + k*6.28318530718);
    vec3 tint = hsv2rgb(vec3(hue, sat, val));

    float ring = smoothstep(0.0, 0.7, sin(log(rD+1e-3)*9.5 + time_f*1.2));
    float pulse = 0.5 + 0.5*sin(time_f*2.0 + rD*28.0 + k*12.0);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m)*ar));
    vign = mix(0.85, 1.2, vign);

    float blendFactor = 0.58;

    float v0 = localVar9(u0, 1.0);
    float v1 = localVar9(u1, 1.0);
    float v2 = localVar9(u2, 1.0);
    float pix = (v0+v1+v2)/3.0;

    float sStrong = smoothstep(0.02, 0.20, pix);
    float radA = mix(0.75, 2.25, sStrong);
    float sigma_r = mix(0.10, 0.22, sStrong);

    vec3 bc0 = bilateral9(u0 + off, radA, sigma_r);
    vec3 bc1 = bilateral9(u1,       radA, sigma_r);
    vec3 bc2 = bilateral9(u2 - off, radA, sigma_r);

    vec3 kaleidoRGB = vec3(bc0.r, bc1.g, bc2.b);

    vec4 kaleidoColor = vec4(kaleidoRGB, 1.0) * vec4(tint, 1.0);
    vec4 merged = mix(kaleidoColor, originalTexture, blendFactor);

    merged.rgb *= (0.75 + 0.25*ring) * (0.85 + 0.15*pulse) * vign;

    vec3 bloom = merged.rgb * merged.rgb * 0.18 + pow(max(merged.rgb-0.6,0.0), vec3(2.0))*0.12;
    vec3 outCol = merged.rgb + bloom;

    float wob = 0.9 + 0.1*sin(time_f + rD*14.0 + k*9.0);
    outCol *= wob;

    vec3 grad = movingGradient(TexCoord, m, time_f, aspect) * 0.5; // 1/2 brightness
    float gradBase = 0.35 + 0.25*sin(time_f*0.5 + rD*7.0 + k*9.0);
    float gradAmt = 0.5 * mix(gradBase*0.5, 0.95, sStrong);   // 1/2 influence
    vec3 screenBlend = 1.0 - (1.0 - outCol) * (1.0 - grad);
    outCol = mix(outCol, screenBlend, gradAmt);

    vec4 t = texture(textTexture, TexCoord);
    outCol = mix(outCol, outCol*t.rgb, 0.8);

    outCol = sin(outCol * (0.5 + 0.5*pingPong(time_f, 12.0)));
    outCol = clamp(outCol, vec3(0.08), vec3(0.96));

    color = vec4(outCol, 1.0);
}
)SHD";
inline const char *src_frac_shader01_smooth = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
float pingPong(float x, float length){
    float m = mod(x, length*2.0);
    return m <= length ? m : length*2.0 - m;
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i=0;i<6;i++){
        p = abs((p - c) * (zoom + 0.15*sin(t*0.35+float(i)))) - 0.5 + c;
        p = rotateUV(p, t*0.12 + float(i)*0.07, c, aspect);
    }
    return p;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 movingGradient(vec2 uv, vec2 c, float t, float aspect){
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    vec2 d = normalize(vec2(cos(t*0.27), sin(t*0.31)));
    float s = dot(p, d);
    float band = 0.5 + 0.5*sin(s*6.28318530718*0.35 + t*0.9);
    float h = fract(s*0.22 + t*0.07 + 0.15*sin(t*0.33));
    float S = 0.75 + 0.25*sin(t*0.21 + s*2.0);
    float V = 0.75 + 0.25*band;
    vec3 base = hsv2rgb(vec3(h, S, V));
    float edge = smoothstep(0.2, 0.8, band);
    return mix(base*0.6, base, edge);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect){
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if(p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

float diamondRadius(vec2 p){
    p = abs(p);
    return max(p.x, p.y);
}

float luma(vec3 c){ return dot(c, vec3(0.299,0.587,0.114)); }

vec3 bilateral9(vec2 uv, float radiusScale, float sigma_r){
    vec2 texel = 1.0 / iResolution;
    vec2 o = texel * radiusScale;
    vec3 c0 = texture(textTexture, uv).rgb;
    float L0 = luma(c0);
    vec2 offs[9] = vec2[](
        vec2(0,0),
        vec2( o.x, 0), vec2(-o.x, 0),
        vec2(0,  o.y), vec2(0, -o.y),
        vec2( o.x,  o.y), vec2(-o.x,  o.y),
        vec2( o.x, -o.y), vec2(-o.x, -o.y)
    );
    float wsum = 0.0;
    vec3 acc = vec3(0.0);
    for(int i=0;i<9;i++){
        vec3 c = texture(textTexture, uv + offs[i]).rgb;
        float dl = luma(c) - L0;
        float wr = exp(-(dl*dl)/(2.0*sigma_r*sigma_r));
        float dsq = dot(offs[i]/texel, offs[i]/texel);
        float ws = exp(-dsq/(2.0*1.0*1.0));
        float w = wr*ws;
        acc += c*w;
        wsum += w;
    }
    return acc / max(wsum, 1e-6);
}

float localVar9(vec2 uv, float radiusScale){
    vec2 texel = 1.0 / iResolution;
    vec2 o = texel * radiusScale;
    float m = 0.0;
    float s = 0.0;
    int n = 0;
    for(int y=-1;y<=1;y++){
        for(int x=-1;x<=1;x++){
            vec3 c = texture(textTexture, uv + vec2(x,y)*o).rgb;
            float L = luma(c);
            m += L;
            s += L*L;
            n++;
        }
    }
    m /= float(n);
    s /= float(n);
    return max(s - m*m, 0.0);
}

void main(){
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = TexCoord;
    vec4 originalTexture = texture(textTexture, TexCoord);

    float seg = 4.0 + 2.0*sin(time_f*0.33);
    vec2 kUV = reflectUV(uv, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = p;
    q = abs(q);
    if(q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18*sin(time_f*0.2);
    float period = log(base);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35*sin(rD*18.0 + time_f*0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap*1.045) / ar + m);
    vec2 u2 = fract((pwrap*0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f*1.3)) * vec2(1.0, 1.0/aspect);

    float hue = fract(ang*0.25 + time_f*0.08 + k*0.5);
    float sat = 0.75 - 0.25*cos(time_f*0.7 + rD*10.0);
    float val = 0.8 + 0.2*sin(time_f*0.9 + k*6.28318530718);
    vec3 tint = hsv2rgb(vec3(hue, sat, val));

    float ring = smoothstep(0.0, 0.7, sin(log(rD+1e-3)*9.5 + time_f*1.2));
    float pulse = 0.5 + 0.5*sin(time_f*2.0 + rD*28.0 + k*12.0);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m)*ar));
    vign = mix(0.85, 1.2, vign);

    float blendFactor = 0.58;

    float v0 = localVar9(u0, 1.0);
    float v1 = localVar9(u1, 1.0);
    float v2 = localVar9(u2, 1.0);
    float pix = (v0+v1+v2)/3.0;

    float sStrong = smoothstep(0.02, 0.20, pix);
    float radA = mix(0.75, 2.25, sStrong);
    float sigma_r = mix(0.10, 0.22, sStrong);

    vec3 bc0 = bilateral9(u0 + off, radA, sigma_r);
    vec3 bc1 = bilateral9(u1,       radA, sigma_r);
    vec3 bc2 = bilateral9(u2 - off, radA, sigma_r);

    float rC = bc0.r;
    float gC = bc1.g;
    float bC = bc2.b;
    vec3 kaleidoRGB = vec3(rC, gC, bC);

    vec4 kaleidoColor = vec4(kaleidoRGB, 1.0) * vec4(tint, 1.0);
    vec4 merged = mix(kaleidoColor, originalTexture, blendFactor);

    merged.rgb *= (0.75 + 0.25*ring) * (0.85 + 0.15*pulse) * vign;

    vec3 bloom = merged.rgb * merged.rgb * 0.18 + pow(max(merged.rgb-0.6,0.0), vec3(2.0))*0.12;
    vec3 outCol = merged.rgb + bloom;

    float wob = 0.9 + 0.1*sin(time_f + rD*14.0 + k*9.0);
    outCol *= wob;

    vec3 grad = movingGradient(TexCoord, m, time_f, aspect);
    float gradBase = 0.35 + 0.25*sin(time_f*0.5 + rD*7.0 + k*9.0);
    float gradAmt = mix(gradBase*0.5, 0.95, sStrong);
    vec3 screenBlend = 1.0 - (1.0 - outCol) * (1.0 - grad);
    outCol = mix(outCol, screenBlend, gradAmt);

    vec4 t = texture(textTexture, TexCoord);
    outCol = mix(outCol, outCol*t.rgb, 0.8);

    outCol = sin(outCol * (0.5 + 0.5*pingPong(time_f, 12.0)));
    outCol = clamp(outCol, vec3(0.08), vec3(0.96));

    color = vec4(outCol, 1.0);
}
)SHD";
inline const char *src_frac_shader01_smooth_neon = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;  
uniform float uamp;
uniform float iTime;
uniform int iFrame; 
uniform float iTimeDelta;
uniform vec4 iDate;
uniform vec2 iMouseClick;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
uniform float iChannelTime[4];
uniform float iSampleRate;

float pingPong(float x, float length){
    float m = mod(x, length*2.0);
    return m <= length ? m : length*2.0 - m;
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i=0;i<6;i++){
        p = abs((p - c) * (zoom + 0.15*sin(t*0.35+float(i)))) - 0.5 + c;
        p = rotateUV(p, t*0.12 + float(i)*0.07, c, aspect);
    }
    return p;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 movingGradient(vec2 uv, vec2 c, float t, float aspect){
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    vec2 d = normalize(vec2(cos(t*0.27), sin(t*0.31)));
    float s = dot(p, d);
    float band = 0.5 + 0.5*sin(s*6.28318530718*0.35 + t*0.9);
    float h = fract(s*0.22 + t*0.07 + 0.15*sin(t*0.33));
    float S = 0.75 + 0.25*sin(t*0.21 + s*2.0);
    float V = 0.75 + 0.25*band;
    vec3 base = hsv2rgb(vec3(h, S, V));
    float edge = smoothstep(0.2, 0.8, band);
    return mix(base*0.6, base, edge);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect){
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if(p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

float diamondRadius(vec2 p){
    p = abs(p);
    return max(p.x, p.y);
}

float luma(vec3 c){ return dot(c, vec3(0.299,0.587,0.114)); }

vec3 bilateral9(vec2 uv, float radiusScale, float sigma_r){
    vec2 texel = 1.0 / iResolution;
    vec2 o = texel * radiusScale;
    vec3 c0 = texture(textTexture, uv).rgb;
    float L0 = luma(c0);
    vec2 offs[9] = vec2[](
        vec2(0,0),
        vec2( o.x, 0), vec2(-o.x, 0),
        vec2(0,  o.y), vec2(0, -o.y),
        vec2( o.x,  o.y), vec2(-o.x,  o.y),
        vec2( o.x, -o.y), vec2(-o.x, -o.y)
    );
    float wsum = 0.0;
    vec3 acc = vec3(0.0);
    for(int i=0;i<9;i++){
        vec3 c = texture(textTexture, uv + offs[i]).rgb;
        float dl = luma(c) - L0;
        float wr = exp(-(dl*dl)/(2.0*sigma_r*sigma_r));
        float dsq = dot(offs[i]/texel, offs[i]/texel);
        float ws = exp(-dsq/(2.0*1.0*1.0));
        float w = wr*ws;
        acc += c*w;
        wsum += w;
    }
    return acc / max(wsum, 1e-6);
}

float localVar9(vec2 uv, float radiusScale){
    vec2 texel = 1.0 / iResolution;
    vec2 o = texel * radiusScale;
    float m = 0.0;
    float s = 0.0;
    int n = 0;
    for(int y=-1;y<=1;y++){
        for(int x=-1;x<=1;x++){
            vec3 c = texture(textTexture, uv + vec2(x,y)*o).rgb;
            float L = luma(c);
            m += L;
            s += L*L;
            n++;
        }
    }
    m /= float(n);
    s /= float(n);
    return max(s - m*m, 0.0);
}

void main(){
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = TexCoord;
    vec4 originalTexture = texture(textTexture, TexCoord);

    float seg = 4.0 + 2.0*sin(time_f*0.33);
    vec2 kUV = reflectUV(uv, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = p;
    q = abs(q);
    if(q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18*sin(time_f*0.2);
    float period = log(base);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35*sin(rD*18.0 + time_f*0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap*1.045) / ar + m);
    vec2 u2 = fract((pwrap*0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f*1.3)) * vec2(1.0, 1.0/aspect);

    float hue = fract(ang*0.25 + time_f*0.08 + k*0.5);
    float sat = 0.75 - 0.25*cos(time_f*0.7 + rD*10.0);
    float val = 0.8 + 0.2*sin(time_f*0.9 + k*6.28318530718);
    vec3 tint = hsv2rgb(vec3(hue, sat, val));

    float ring = smoothstep(0.0, 0.7, sin(log(rD+1e-3)*9.5 + time_f*1.2));
    float pulse = 0.5 + 0.5*sin(time_f*2.0 + rD*28.0 + k*12.0);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m)*ar));
    vign = mix(0.85, 1.2, vign);

    float blendFactor = 0.58;

    float v0 = localVar9(u0, 1.0);
    float v1 = localVar9(u1, 1.0);
    float v2 = localVar9(u2, 1.0);
    float pix = (v0+v1+v2)/3.0;

    float sStrong = smoothstep(0.02, 0.20, pix);
    float radA = mix(0.75, 2.25, sStrong);
    float sigma_r = mix(0.10, 0.22, sStrong);

    vec3 bc0 = bilateral9(u0 + off, radA, sigma_r);
    vec3 bc1 = bilateral9(u1,       radA, sigma_r);
    vec3 bc2 = bilateral9(u2 - off, radA, sigma_r);

    float rC = bc0.r;
    float gC = bc1.g;
    float bC = bc2.b;
    vec3 kaleidoRGB = vec3(rC, gC, bC);

    vec4 kaleidoColor = vec4(kaleidoRGB, 1.0) * vec4(tint, 1.0);
    vec4 merged = mix(kaleidoColor, originalTexture, blendFactor);

    merged.rgb *= (0.75 + 0.25*ring) * (0.85 + 0.15*pulse) * vign;

    vec3 bloom = merged.rgb * merged.rgb * 0.18 + pow(max(merged.rgb-0.6,0.0), vec3(2.0))*0.12;
    vec3 outCol = merged.rgb + bloom;

    float wob = 0.9 + 0.1*sin(time_f + rD*14.0 + k*9.0);
    outCol *= wob;

    vec3 grad = movingGradient(TexCoord, m, time_f, aspect);
    float gradBase = 0.35 + 0.25*sin(time_f*0.5 + rD*7.0 + k*9.0);
    float gradAmt = mix(gradBase*0.5, 0.95, sStrong);
    vec3 screenBlend = 1.0 - (1.0 - outCol) * (1.0 - grad);
    outCol = mix(outCol, screenBlend, gradAmt);

    vec4 t = texture(textTexture, TexCoord);
    outCol = mix(outCol, outCol*t.rgb, 0.8);

    float lum = dot(outCol, vec3(0.299, 0.587, 0.114));
    float tHue = time_f * 0.15 + uamp * 0.1;
    float hueBase = fract(lum * 0.8 + tHue);
    vec3 neon1 = hsv2rgb(vec3(hueBase, 1.0, 1.0));
    vec3 neon2 = hsv2rgb(vec3(fract(hueBase + 0.33), 1.0, 1.0));
    float wave = pingPong(time_f * 0.25 + amp * 2.0, 1.0);
    vec3 neon = mix(neon1, neon2, wave);

    float audio = clamp(amp * 1.5 + uamp * 0.25, 0.0, 1.5);
    float strength = clamp(0.2 + audio, 0.0, 1.0);

    vec3 mixed = mix(outCol, neon, strength);
    mixed = pow(mixed, vec3(0.8));
    mixed = clamp(mixed, 0.0, 1.0);

    color = vec4(mixed, 1.0);
}
)SHD";
inline const char *src_frac_shader02_dmd = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length){
    float m = mod(x, length*2.0);
    return m <= length ? m : length*2.0 - m;
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect){
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect){
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect){
    vec2 p = uv;
    for(int i=0;i<6;i++){
        p = abs((p - c) * (zoom + 0.15*sin(t*0.35+float(i)))) - 0.5 + c;
        p = rotateUV(p, t*0.12 + float(i)*0.07, c, aspect);
    }
    return p;
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 movingGradient(vec2 uv, vec2 c, float t, float aspect){
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (uv - c) * ar;
    vec2 d = normalize(vec2(cos(t*0.27), sin(t*0.31)));
    float s = dot(p, d);
    float band = 0.5 + 0.5*sin(s*6.28318530718*0.35 + t*0.9);
    float h = fract(s*0.22 + t*0.07 + 0.15*sin(t*0.33));
    float S = 0.75 + 0.25*sin(t*0.21 + s*2.0);
    float V = 0.75 + 0.25*band;
    vec3 base = hsv2rgb(vec3(h, S, V));
    float edge = smoothstep(0.2, 0.8, band);
    return mix(base*0.6, base, edge);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect){
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if(p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

float diamondRadius(vec2 p){
    p = abs(p);
    return max(p.x, p.y);
}

void main(){
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = TexCoord;
    vec4 originalTexture = texture(textTexture, TexCoord);

    float seg = 4.0 + 2.0*sin(time_f*0.33);
    vec2 kUV = reflectUV(uv, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = p;
    q = abs(q);
    if(q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18*sin(time_f*0.2);
    float period = log(base);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35*sin(rD*18.0 + time_f*0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap*1.045) / ar + m);
    vec2 u2 = fract((pwrap*0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f*1.3)) * vec2(1.0, 1.0/aspect);

    float hue = fract(ang*0.25 + time_f*0.08 + k*0.5);
    float sat = 0.75 - 0.25*cos(time_f*0.7 + rD*10.0);
    float val = 0.8 + 0.2*sin(time_f*0.9 + k*6.28318530718);
    vec3 tint = hsv2rgb(vec3(hue, sat, val));

    float ring = smoothstep(0.0, 0.7, sin(log(rD+1e-3)*9.5 + time_f*1.2));
    float pulse = 0.5 + 0.5*sin(time_f*2.0 + rD*28.0 + k*12.0);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m)*ar));
    vign = mix(0.85, 1.2, vign);

    float blendFactor = 0.58;

    float rC = texture(textTexture, u0 + off).r;
    float gC = texture(textTexture, u1).g;
    float bC = texture(textTexture, u2 - off).b;
    vec3 kaleidoRGB = vec3(rC, gC, bC);

    vec4 kaleidoColor = vec4(kaleidoRGB, 1.0) * vec4(tint, 1.0);
    vec4 merged = mix(kaleidoColor, originalTexture, blendFactor);

    merged.rgb *= (0.75 + 0.25*ring) * (0.85 + 0.15*pulse) * vign;

    vec3 bloom = merged.rgb * merged.rgb * 0.18 + pow(max(merged.rgb-0.6,0.0), vec3(2.0))*0.12;
    vec3 outCol = merged.rgb + bloom;

    float wob = 0.9 + 0.1*sin(time_f + rD*14.0 + k*9.0);
    outCol *= wob;

    vec3 grad = movingGradient(TexCoord, m, time_f, aspect);
    float gradAmt = 0.35 + 0.25*sin(time_f*0.5 + rD*7.0 + k*9.0);
    vec3 screenBlend = 1.0 - (1.0 - outCol) * (1.0 - grad);
    outCol = mix(outCol, screenBlend, gradAmt);

    vec4 t = texture(textTexture, TexCoord);
    outCol = mix(outCol, outCol*t.rgb, 0.8);

    outCol = sin(outCol * (0.5 + 0.5*pingPong(-time_f * PI, 5.0)));
    outCol = clamp(outCol, vec3(0.08), vec3(0.96));

    

    color = mix(t, vec4(outCol, 1.0),-pingPong(time_f *
 PI, 5.0));

}
)SHD";
inline const char *src_frac_shader02_dmd_mandella = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float seed;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv, float tSlow, float tFast, float aspect) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    vec2 gdir = normalize(vec2(cos(tSlow * 0.27), sin(tSlow * 0.31)));
    float s = dot((uv - 0.5) * vec2(aspect, 1.0), gdir);
    float w = max(fwidth(s) * 4.0, 0.002);
    float band = smoothstep(-0.5 - w, -0.5 + w, sin(s * 2.2 + tFast * 0.9));
    vec3 neon = neonPalette(tFast);
    vec3 grad = mix(tex, mix(tex, neon, 0.6), 0.35 + 0.25 * band);
    grad = mix(grad, tex, 0.10);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 rot2(vec2 v, float a) {
    float c = cos(a), s = sin(a);
    return vec2(c * v.x - s * v.y, s * v.x + c * v.y);
}

vec2 h2(vec2 p) {
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)),
                          dot(p, vec2(269.5, 183.3)))) * 43758.5453);
}

vec3 limitHighlights(vec3 c) {
    float m = max(c.r, max(c.g, c.b));
    if (m > 0.9) c *= 0.9 / m;
    return c;
}

// Spiral / sector warp from the extra shader, amplitude–aware.
vec2 spiralWarp(vec2 tcIn, float tBase, float a01, out float ringMirrorOut) {
    float loopDuration = 25.0;
    float t = mod(tBase * (0.6 + 1.8 * a01), loopDuration);
    vec2 aspect2 = vec2(iResolution.x / iResolution.y, 1.0);

    vec2 nc = (tcIn * 2.0 - 1.0) * aspect2;
    nc.x = abs(nc.x);
    float d = length(nc);
    float a = atan(nc.y, nc.x);

    float spiralSpeed = 5.0 * (0.7 + 1.5 * a01);
    float inward = (t / loopDuration) * (0.5 + 0.5 * a01);

    a += (1.0 - smoothstep(0.0, 8.0, d)) * t * spiralSpeed;
    d *= 1.0 - inward;

    vec2 spiral = vec2(cos(a), sin(a)) * tan(d);
    vec2 uv0 = (spiral / aspect2 + 1.0) * 0.5;

    vec2 p = (uv0 * 2.0 - 1.0) * aspect2;
    float r = length(p);
    float ang = atan(p.y, p.x);

    float N = mix(8.0, 16.0, a01);
    float tau = 6.28318530718;
    float sector = tau / N;
    ang = mod(ang + 0.5 * sector, sector);
    ang = abs(ang - 0.5 * sector);

    float ringFreq = 6.0 + 4.0 * a01;
    float ring = fract(r * ringFreq + 0.15 * sin(tBase * 0.5));
    float ringMirror = abs(ring - 0.5) * 2.0;

    float swirl = 0.25 * sin(tBase * 0.3) * (0.5 + 1.0 * a01);
    ang += swirl * r;

    float zoom = 0.85 + 0.1 * sin(tBase * 0.27) + 0.08 * a01;
    vec2 m = vec2(cos(ang), sin(ang)) * (r * zoom * (0.85 + 0.15 * ringMirror));

    vec2 uv = (m / aspect2 + 1.0) * 0.5;
    ringMirrorOut = ringMirror;
    return uv;
}

void main(void) {
    float a = clamp(amp, 0.0, 1.0);
    float ua = clamp(uamp, 0.0, 1.0);
    float aMix = clamp(amp * 0.7 + uamp * 1.3, 0.0, 4.0);
    float a01 = clamp(aMix / 2.5, 0.0, 1.0);

    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);

    float tBase = time_f;
    float tSlow = tBase * mix(0.2, 0.9, a01);
    float tFast = tBase * mix(0.7, 3.5, a01);

    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution)
                              : fract(vec2(0.37 + 0.11 * sin(tSlow * 0.63 + seed),
                                           0.42 + 0.13 * cos(tSlow * 0.57 + seed * 2.0)));

    // Base coordinates from original screen
    vec2 tc0 = TexCoord;
    vec2 center = vec2(0.5, 0.5);
    float radius = length(tc0 - center);

    // Ripple + twist (driven by amplitude)
    float rippleSpeed = 5.0 * (1.0 + 2.0 * a01);
    float rippleAmplitude = 0.03 * (0.5 + 1.5 * a01);
    float rippleWavelength = 10.0;
    float twistStrength = 1.0 + 4.0 * a01;

    float ripple = sin(tc0.x * rippleWavelength + tFast * rippleSpeed) * rippleAmplitude;
    ripple += sin(tc0.y * rippleWavelength + tFast * rippleSpeed) * rippleAmplitude;
    vec2 rippleTC = tc0 + vec2(ripple, ripple);

    float angleTwist = twistStrength * (radius - 1.0) + tSlow;
    float cosA = cos(angleTwist);
    float sinA = sin(angleTwist);
    mat2 rotationMatrix = mat2(cosA, -sinA, sinA, cosA);
    vec2 twistedTC = rotationMatrix * (tc0 - center) + center;

    float mixRT = 0.5 + 0.35 * a01;
    vec2 tcRT = mix(rippleTC, twistedTC, mixRT);

    // Spiral warp on top of ripple+twist
    float ringMirrorSpiral;
    vec2 spiralTC = spiralWarp(tcRT, tBase, a01, ringMirrorSpiral);

    vec4 baseTex = texture(textTexture, spiralTC);

    // Glow radius based on spiral coordinates
    vec2 uvForGlow = spiralTC * 2.0 - 1.0;
    uvForGlow.x *= aspect;
    float rGlow = pingPong(sin(length(uvForGlow) * tSlow), 5.0);
    float radiusMax = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radiusMax, radiusMax - 0.25, rGlow);
    glow *= (0.8 + 0.4 * ringMirrorSpiral);

    // Use spiral output as main UV for chroma air + kaleido
    vec2 uv = spiralTC;
    float speedScale = 1.0 + 2.0 * a01 + 3.0 * ua;

    float speedR = 5.0 * speedScale;
    float speedG = 6.5 * speedScale;
    float speedB = 4.0 * speedScale;
    float ampR   = 0.03 * (0.7 + 0.6 * a01);
    float ampG   = 0.025 * (0.7 + 0.6 * a01);
    float ampB   = 0.035 * (0.7 + 0.6 * a01);
    float waveR  = 10.0;
    float waveG  = 12.0;
    float waveB  = 8.0;

    float rR = sin(uv.x * waveR        + tFast * speedR) * ampR
             + sin(uv.y * waveR * 0.8  + tFast * speedR * 1.2) * ampR;
    float rG = sin(uv.x * waveG * 1.5  + tFast * speedG) * ampG
             + sin(uv.y * waveG * 0.3  + tFast * speedG * 0.7) * ampG;
    float rB = sin(uv.x * waveB * 0.5  + tFast * speedB) * ampB
             + sin(uv.y * waveB * 1.7  + tFast * speedB * 1.3) * ampB;

    vec2 tcR = uv + vec2(rR, rR);
    vec2 tcG = uv + vec2(rG, -0.5 * rG);
    vec2 tcB = uv + vec2(0.3 * rB, rB);

    vec3 pats[4] = vec3[](vec3(1,0,1), vec3(0,1,0), vec3(1,0,0), vec3(0,0,1));
    float pspd = 4.0;
    int pidx = int(mod(floor(tBase * pspd + seed * 4.0), 4.0));
    vec3 mir = pats[pidx];

    vec2 dR = tcR - m;
    vec2 dG = tcG - m;
    vec2 dB = tcB - m;

    float fallR = smoothstep(0.55, 0.0, length(dR));
    float fallG = smoothstep(0.55, 0.0, length(dG));
    float fallB = smoothstep(0.55, 0.0, length(dB));

    float sw = (0.12 + 0.38 * ua + 0.25 * a);
    vec2 tangR = rot2(normalize(dR + 1e-4), 1.5707963);
    vec2 tangG = rot2(normalize(dG + 1e-4), 1.5707963);
    vec2 tangB = rot2(normalize(dB + 1e-4), 1.5707963);

    vec2 airR = tangR * sw * fallR * (0.06 + 0.22 * a) * (0.6 + 0.4 * cos(uv.y * 40.0 + tFast * 3.0 + seed));
    vec2 airG = tangG * sw * fallG * (0.06 + 0.22 * a) * (0.6 + 0.4 * cos(uv.y * 38.0 + tFast * 3.3 + seed * 1.7));
    vec2 airB = tangB * sw * fallB * (0.06 + 0.22 * a) * (0.6 + 0.4 * cos(uv.y * 42.0 + tFast * 2.9 + seed * 0.9));

    vec2 jit = (h2(uv * vec2(233.3, 341.9) + tFast + seed) - 0.5)
             * (0.0006 + 0.004 * ua);

    tcR += airR + jit;
    tcG += airG + jit;
    tcB += airB + jit;

    vec2 fR = vec2(mir.r > 0.5 ? 1.0 - tcR.x : tcR.x, tcR.y);
    vec2 fG = vec2(mir.g > 0.5 ? 1.0 - tcG.x : tcG.x, tcG.y);
    vec2 fB = vec2(mir.b > 0.5 ? 1.0 - tcB.x : tcB.x, tcB.y);

    float seg = 4.0 + 2.0 * sin(tSlow * 0.33 + a01 * 2.0);
    vec2 kUV = reflectUV(spiralTC, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(tSlow * 0.42 + a01 * 3.0);
    kUV = fractalFold(kUV, foldZoom, tFast, m, aspect);
    kUV = rotateUV(kUV, tSlow * 0.23 + a01 * 1.1, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(tSlow * 0.2) * (PI * tSlow), 5.0);
    float period = log(base) * pingPong(tSlow * PI, 5.0);
    float tz = tSlow * 0.65;
    float rD = diamondRadius(p) + 1e-6;

    float ang = atan(q.y, q.x) + tz * 0.35
              + 0.35 * sin(rD * 18.0 + tFast * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);

    vec2 dirK = normalize(pwrap + 1e-6);
    vec2 off = dirK * (0.0015 + 0.001 * sin(tFast * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((spiralTC - m) * ar));
    vign = mix(0.9, 1.15, vign) * (0.8 + 0.3 * ringMirrorSpiral);

    vec2 sR = mix(u0, fR, 0.6);
    vec2 sG = mix(u1, fG, 0.6);
    vec2 sB = mix(u2, fB, 0.6);

    vec3 rC = preBlendColor(sR + off, tSlow, tFast, aspect);
    vec3 gC = preBlendColor(sG,       tSlow, tFast, aspect);
    vec3 bC = preBlendColor(sB - off, tSlow, tFast, aspect);

    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + tFast * 1.2));
    ring = ring * pingPong(tSlow * PI, 5.0) * (0.7 + 0.5 * ringMirrorSpiral);
    float pulse = 0.5 + 0.5 * sin(tFast * 2.0 + rD * 28.0 + k * 12.0);
    pulse *= mix(0.4, 1.4, a01);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.10
               + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.06;

    float pulseAdd = 0.004 * (0.5 + 0.5 * sin(tFast * 3.7 + seed));
    outCol += bloom;
    outCol += pulseAdd * ua;

    outCol = clamp(outCol, vec3(0.0), vec3(1.0));
    outCol = limitHighlights(outCol);

    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * (0.6 + 0.3 * a01));
    finalRGB = limitHighlights(finalRGB);
    finalRGB = clamp(finalRGB, 0.0, 1.0);

    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd2 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = abs(p);
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = length(uv);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * sin(time_f * 0.2);
    float period = log(base);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;
    outCol = mix(outCol, baseCol, 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd2_amp = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv, float t) {
    float aspect = iResolution.x / iResolution.y;
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    vec2 gdir = normalize(vec2(cos(t * 0.27), sin(t * 0.31)));
    float s = dot((uv - 0.5) * vec2(aspect, 1.0), gdir);
    float w = max(fwidth(s) * 4.0, 0.002);
    float band = smoothstep(-0.5 - w, -0.5 + w, sin(s * 2.2 + t * 0.9));
    vec3 neon = neonPalette(t);
    vec3 grad = mix(tex, mix(tex, neon, 0.6), 0.35 + 0.25 * band);
    grad = mix(grad, tex, 0.10);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = abs(p);
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec3 limitHighlights(vec3 c) {
    float m = max(c.r, max(c.g, c.b));
    if (m > 0.95) c *= 0.95 / m;
    return c;
}

void main(void) {
    float aAcc = clamp(amp, 0.0, 20.0);
    float aInst = clamp(uamp, 0.0, 20.0);
    float aMix = clamp(aAcc * 0.5 + aInst * 1.0, 0.0, 20.0);
    float a01 = clamp(aMix / 6.0, 0.0, 1.0);

    float aspect = iResolution.x / iResolution.y;
    vec2 center = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5, 0.5);

    float tSpin  = time_f * mix(0.3, 1.8, a01);
    float tSlow  = time_f * mix(0.2, 0.9, a01);
    float tFast  = time_f * mix(0.8, 3.2, a01);

    vec2 offset = TexCoord - center;
    float maxRadius = length(vec2(0.5, 0.5));
    float radius = length(offset);
    float normalizedRadius = radius / maxRadius;
    float angle = atan(offset.y, offset.x);

    float distortion = 0.25 + 0.75 * a01;
    float distortedRadius = normalizedRadius + distortion * normalizedRadius * normalizedRadius;
    distortedRadius = clamp(distortedRadius, 0.0, 1.0);
    distortedRadius *= maxRadius;

    vec2 distortedCoords = center + distortedRadius * vec2(cos(angle), sin(angle));

    float modulatedTime = pingPong(tSpin, 5.0);
    angle += modulatedTime;

    vec2 rotatedTC;
    rotatedTC.x = cos(angle) * (distortedCoords.x - center.x)
                - sin(angle) * (distortedCoords.y - center.y) + center.x;
    rotatedTC.y = sin(angle) * (distortedCoords.x - center.x)
                + cos(angle) * (distortedCoords.y - center.y) + center.y;

    float warpSpeed = 0.05 + 0.25 * a01 + 0.2 * (aInst / (aInst + 1.0));
    vec2 warpedCoords;
    warpedCoords.x = pingPong(rotatedTC.x + tSpin * warpSpeed, 1.0);
    warpedCoords.y = pingPong(rotatedTC.y + tSpin * warpSpeed, 1.0);

    vec2 uvGlow = warpedCoords * 2.0 - 1.0;
    uvGlow.x *= aspect;
    float rGlow = length(uvGlow);
    float radiusMax = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radiusMax, radiusMax - 0.25, rGlow);

    vec2 m = center;
    vec2 ar = vec2(aspect, 1.0);

    vec3 baseColBlur = preBlendColor(warpedCoords, tSlow);
    float seg = 4.0 + 2.0 * sin(tSlow * 0.33 + a01 * 2.0);

    vec2 kUV = reflectUV(warpedCoords, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    float foldZoom = 1.45 + 0.55 * sin(tSlow * 0.42 + a01 * 3.0);
    kUV = fractalFold(kUV, foldZoom, tFast, m, aspect);
    kUV = rotateUV(kUV, tSlow * 0.23 + a01 * 1.1, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * sin(tSlow * 0.2);
    float period = log(base);
    float tz = tSlow * 0.65;
    float rD = diamondRadius(p) + 1e-6;

    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + tFast * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(tFast * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((warpedCoords - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off, tFast);
    vec3 gC = preBlendColor(u1,       tFast);
    vec3 bC = preBlendColor(u2 - off, tFast);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + tFast * 1.2));
    float pulse = 0.5 + 0.5 * sin(tFast * 2.0 + rD * 28.0 + k * 12.0);
    pulse *= mix(0.4, 1.4, a01);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.10
               + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.06;
    outCol += bloom;

    outCol = mix(outCol, baseColBlur, 0.18 + 0.3 * a01);
    outCol = limitHighlights(outCol);
    outCol = clamp(outCol, 0.0, 1.0);

    vec4 baseTex = texture(textTexture, warpedCoords);
    float mixAmt = pingPong(glow * PI, 5.0) * mix(0.4, 0.9, a01);
    vec3 finalRGB = mix(baseTex.rgb, outCol, mixAmt);
    finalRGB = limitHighlights(finalRGB);
    finalRGB = clamp(finalRGB, 0.0, 1.0);

    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd3 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = abs(p);
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = length(uv);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * sin(time_f * 0.2);
    float period = log(base);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse *  PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd4 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = abs(p);
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = length(uv);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * sin(time_f * 0.2);
    float period = log(base);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    rw =  pingPong(sin(rw * (time_f * PI)), 5.0);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse *  (PI * time_f), 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd5 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = abs(p);
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = length(uv);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * sin(time_f * 0.2);
    float period = log(base);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);

    pulse = (pulse * PI) *  pingPong(time_f, 5.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse *  PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd6i = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0); 
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;

    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse *  PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd6i_air = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float seed;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv, float tSlow, float tFast, float aspect) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    vec2 gdir = normalize(vec2(cos(tSlow * 0.27), sin(tSlow * 0.31)));
    float s = dot((uv - 0.5) * vec2(aspect, 1.0), gdir);
    float w = max(fwidth(s) * 4.0, 0.002);
    float band = smoothstep(-0.5 - w, -0.5 + w, sin(s * 2.2 + tFast * 0.9));
    vec3 neon = neonPalette(tFast);
    vec3 grad = mix(tex, mix(tex, neon, 0.6), 0.35 + 0.25 * band);
    grad = mix(grad, tex, 0.10);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 rot2(vec2 v, float a){
    float c = cos(a), s = sin(a);
    return vec2(c*v.x - s*v.y, s*v.x + c*v.y);
}

vec2 h2(vec2 p){
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),
                          dot(p,vec2(269.5,183.3))))*43758.5453);
}

vec3 limitHighlights(vec3 c){
    float m = max(c.r, max(c.g, c.b));
    if(m > 0.9) c *= 0.9 / m;
    return c;
}

void main(void) {
    float a = clamp(amp, 0.0, 1.0);
    float ua = clamp(uamp, 0.0, 1.0);
    float aMix = clamp(amp * 0.7 + uamp * 1.3, 0.0, 4.0);
    float a01 = clamp(aMix / 2.5, 0.0, 1.0);

    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);

    float tBase = time_f;
    float tSlow = tBase * mix(0.2, 0.9, a01);
    float tFast = tBase * mix(0.7, 3.5, a01);

    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution)
                              : fract(vec2(0.37 + 0.11 * sin(tSlow*0.63 + seed),
                                           0.42 + 0.13 * cos(tSlow*0.57 + seed*2.0)));

    vec4 baseTex = texture(textTexture, TexCoord);

    vec2 uvForGlow = TexCoord * 2.0 - 1.0;
    uvForGlow.x *= aspect;
    float rGlow = pingPong(sin(length(uvForGlow) * tSlow), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, rGlow);

    vec2 uv = TexCoord;
    float speedScale = 1.0 + 2.0 * a01 + 3.0 * ua;

    float speedR = 5.0  * speedScale;
    float speedG = 6.5  * speedScale;
    float speedB = 4.0  * speedScale;
    float ampR   = 0.03 * (0.7 + 0.6 * a01);
    float ampG   = 0.025 * (0.7 + 0.6 * a01);
    float ampB   = 0.035 * (0.7 + 0.6 * a01);
    float waveR  = 10.0;
    float waveG  = 12.0;
    float waveB  = 8.0;

    float rR = sin(uv.x*waveR        + tFast*speedR)*ampR
             + sin(uv.y*waveR*0.8    + tFast*speedR*1.2)*ampR;
    float rG = sin(uv.x*waveG*1.5    + tFast*speedG)*ampG
             + sin(uv.y*waveG*0.3    + tFast*speedG*0.7)*ampG;
    float rB = sin(uv.x*waveB*0.5    + tFast*speedB)*ampB
             + sin(uv.y*waveB*1.7    + tFast*speedB*1.3)*ampB;

    vec2 tcR = uv + vec2(rR, rR);
    vec2 tcG = uv + vec2(rG, -0.5*rG);
    vec2 tcB = uv + vec2(0.3*rB, rB);

    vec3 pats[4] = vec3[](vec3(1,0,1), vec3(0,1,0), vec3(1,0,0), vec3(0,0,1));
    float pspd = 4.0;
    int pidx = int(mod(floor(tBase*pspd + seed*4.0), 4.0));
    vec3 mir = pats[pidx];

    vec2 dR = tcR - m;
    vec2 dG = tcG - m;
    vec2 dB = tcB - m;

    float fallR = smoothstep(0.55, 0.0, length(dR));
    float fallG = smoothstep(0.55, 0.0, length(dG));
    float fallB = smoothstep(0.55, 0.0, length(dB));

    float sw = (0.12 + 0.38*ua + 0.25*a);
    vec2 tangR = rot2(normalize(dR + 1e-4), 1.5707963);
    vec2 tangG = rot2(normalize(dG + 1e-4), 1.5707963);
    vec2 tangB = rot2(normalize(dB + 1e-4), 1.5707963);

    vec2 airR = tangR * sw * fallR * (0.06+0.22*a) * (0.6+0.4*cos(uv.y*40.0 + tFast*3.0 + seed));
    vec2 airG = tangG * sw * fallG * (0.06+0.22*a) * (0.6+0.4*cos(uv.y*38.0 + tFast*3.3 + seed*1.7));
    vec2 airB = tangB * sw * fallB * (0.06+0.22*a) * (0.6+0.4*cos(uv.y*42.0 + tFast*2.9 + seed*0.9));

    vec2 jit = (h2(uv*vec2(233.3,341.9) + tFast + seed) - 0.5)
             * (0.0006 + 0.004*ua);

    tcR += airR + jit;
    tcG += airG + jit;
    tcB += airB + jit;

    vec2 fR = vec2(mir.r>0.5 ? 1.0 - tcR.x : tcR.x, tcR.y);
    vec2 fG = vec2(mir.g>0.5 ? 1.0 - tcG.x : tcG.x, tcG.y);
    vec2 fB = vec2(mir.b>0.5 ? 1.0 - tcB.x : tcB.x, tcB.y);

    float seg = 4.0 + 2.0 * sin(tSlow * 0.33 + a01 * 2.0);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(tSlow * 0.42 + a01 * 3.0);
    kUV = fractalFold(kUV, foldZoom, tFast, m, aspect);
    kUV = rotateUV(kUV, tSlow * 0.23 + a01 * 1.1, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(tSlow * 0.2) * (PI * tSlow), 5.0);
    float period = log(base) * pingPong(tSlow * PI, 5.0);
    float tz = tSlow * 0.65;
    float rD = diamondRadius(p) + 1e-6;

    float ang = atan(q.y, q.x) + tz * 0.35
              + 0.35 * sin(rD * 18.0 + tFast * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);

    vec2 dirK = normalize(pwrap + 1e-6);
    vec2 off = dirK * (0.0015 + 0.001 * sin(tFast * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec2 sR = mix(u0, fR, 0.6);
    vec2 sG = mix(u1, fG, 0.6);
    vec2 sB = mix(u2, fB, 0.6);

    vec3 rC = preBlendColor(sR + off, tSlow, tFast, aspect);
    vec3 gC = preBlendColor(sG,       tSlow, tFast, aspect);
    vec3 bC = preBlendColor(sB - off, tSlow, tFast, aspect);

    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + tFast * 1.2));
    ring = ring * pingPong(tSlow * PI, 5.0);
    float pulse = 0.5 + 0.5 * sin(tFast * 2.0 + rD * 28.0 + k * 12.0);
    pulse *= mix(0.4, 1.4, a01);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.10
               + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.06;

    float pulseAdd = 0.004 * (0.5 + 0.5 * sin(tFast * 3.7 + seed));
    outCol += bloom;
    outCol += pulseAdd * ua;

    outCol = clamp(outCol, vec3(0.0), vec3(1.0));
    outCol = limitHighlights(outCol);

    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * (0.6 + 0.3 * a01));
    finalRGB = limitHighlights(finalRGB);
    finalRGB = clamp(finalRGB, 0.0, 1.0);

    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd6i_air_twist = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float seed;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv, float tSlow, float tFast, float aspect) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    vec2 gdir = normalize(vec2(cos(tSlow * 0.27), sin(tSlow * 0.31)));
    float s = dot((uv - 0.5) * vec2(aspect, 1.0), gdir);
    float w = max(fwidth(s) * 4.0, 0.002);
    float band = smoothstep(-0.5 - w, -0.5 + w, sin(s * 2.2 + tFast * 0.9));
    vec3 neon = neonPalette(tFast);
    vec3 grad = mix(tex, mix(tex, neon, 0.6), 0.35 + 0.25 * band);
    grad = mix(grad, tex, 0.10);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

vec2 rot2(vec2 v, float a) {
    float c = cos(a), s = sin(a);
    return vec2(c * v.x - s * v.y, s * v.x + c * v.y);
}

vec2 h2(vec2 p) {
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)),
                          dot(p, vec2(269.5, 183.3)))) * 43758.5453);
}

vec3 limitHighlights(vec3 c) {
    float m = max(c.r, max(c.g, c.b));
    if (m > 0.9) c *= 0.9 / m;
    return c;
}

void main(void) {
    float a = clamp(amp, 0.0, 1.0);
    float ua = clamp(uamp, 0.0, 1.0);
    float aMix = clamp(amp * 0.7 + uamp * 1.3, 0.0, 4.0);
    float a01 = clamp(aMix / 2.5, 0.0, 1.0);

    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);

    float tBase = time_f;
    float tSlow = tBase * mix(0.2, 0.9, a01);
    float tFast = tBase * mix(0.7, 3.5, a01);

    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution)
                              : fract(vec2(0.37 + 0.11 * sin(tSlow * 0.63 + seed),
                                           0.42 + 0.13 * cos(tSlow * 0.57 + seed * 2.0)));

    vec2 tc0 = TexCoord;
    vec2 center = vec2(0.5, 0.5);
    float radius = length(tc0 - center);

    float rippleSpeed = 5.0 * (1.0 + 2.0 * a01);
    float rippleAmplitude = 0.03 * (0.5 + 1.5 * a01);
    float rippleWavelength = 10.0;
    float twistStrength = 1.0 + 4.0 * a01;

    float ripple = sin(tc0.x * rippleWavelength + tFast * rippleSpeed) * rippleAmplitude;
    ripple += sin(tc0.y * rippleWavelength + tFast * rippleSpeed) * rippleAmplitude;
    vec2 rippleTC = tc0 + vec2(ripple, ripple);

    float angleTwist = twistStrength * (radius - 1.0) + tSlow;
    float cosA = cos(angleTwist);
    float sinA = sin(angleTwist);
    mat2 rotationMatrix = mat2(cosA, -sinA, sinA, cosA);
    vec2 twistedTC = rotationMatrix * (tc0 - center) + center;

    float mixRT = 0.5 + 0.35 * a01;
    vec2 tcRT = mix(rippleTC, twistedTC, mixRT);

    vec4 baseTex = texture(textTexture, tcRT);

    vec2 uvForGlow = tcRT * 2.0 - 1.0;
    uvForGlow.x *= aspect;
    float rGlow = pingPong(sin(length(uvForGlow) * tSlow), 5.0);
    float radiusMax = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radiusMax, radiusMax - 0.25, rGlow);

    vec2 uv = tcRT;
    float speedScale = 1.0 + 2.0 * a01 + 3.0 * ua;

    float speedR = 5.0 * speedScale;
    float speedG = 6.5 * speedScale;
    float speedB = 4.0 * speedScale;
    float ampR   = 0.03 * (0.7 + 0.6 * a01);
    float ampG   = 0.025 * (0.7 + 0.6 * a01);
    float ampB   = 0.035 * (0.7 + 0.6 * a01);
    float waveR  = 10.0;
    float waveG  = 12.0;
    float waveB  = 8.0;

    float rR = sin(uv.x * waveR        + tFast * speedR) * ampR
             + sin(uv.y * waveR * 0.8  + tFast * speedR * 1.2) * ampR;
    float rG = sin(uv.x * waveG * 1.5  + tFast * speedG) * ampG
             + sin(uv.y * waveG * 0.3  + tFast * speedG * 0.7) * ampG;
    float rB = sin(uv.x * waveB * 0.5  + tFast * speedB) * ampB
             + sin(uv.y * waveB * 1.7  + tFast * speedB * 1.3) * ampB;

    vec2 tcR = uv + vec2(rR, rR);
    vec2 tcG = uv + vec2(rG, -0.5 * rG);
    vec2 tcB = uv + vec2(0.3 * rB, rB);

    vec3 pats[4] = vec3[](vec3(1,0,1), vec3(0,1,0), vec3(1,0,0), vec3(0,0,1));
    float pspd = 4.0;
    int pidx = int(mod(floor(tBase * pspd + seed * 4.0), 4.0));
    vec3 mir = pats[pidx];

    vec2 dR = tcR - m;
    vec2 dG = tcG - m;
    vec2 dB = tcB - m;

    float fallR = smoothstep(0.55, 0.0, length(dR));
    float fallG = smoothstep(0.55, 0.0, length(dG));
    float fallB = smoothstep(0.55, 0.0, length(dB));

    float sw = (0.12 + 0.38 * ua + 0.25 * a);
    vec2 tangR = rot2(normalize(dR + 1e-4), 1.5707963);
    vec2 tangG = rot2(normalize(dG + 1e-4), 1.5707963);
    vec2 tangB = rot2(normalize(dB + 1e-4), 1.5707963);

    vec2 airR = tangR * sw * fallR * (0.06 + 0.22 * a) * (0.6 + 0.4 * cos(uv.y * 40.0 + tFast * 3.0 + seed));
    vec2 airG = tangG * sw * fallG * (0.06 + 0.22 * a) * (0.6 + 0.4 * cos(uv.y * 38.0 + tFast * 3.3 + seed * 1.7));
    vec2 airB = tangB * sw * fallB * (0.06 + 0.22 * a) * (0.6 + 0.4 * cos(uv.y * 42.0 + tFast * 2.9 + seed * 0.9));

    vec2 jit = (h2(uv * vec2(233.3, 341.9) + tFast + seed) - 0.5)
             * (0.0006 + 0.004 * ua);

    tcR += airR + jit;
    tcG += airG + jit;
    tcB += airB + jit;

    vec2 fR = vec2(mir.r > 0.5 ? 1.0 - tcR.x : tcR.x, tcR.y);
    vec2 fG = vec2(mir.g > 0.5 ? 1.0 - tcG.x : tcG.x, tcG.y);
    vec2 fB = vec2(mir.b > 0.5 ? 1.0 - tcB.x : tcB.x, tcB.y);

    float seg = 4.0 + 2.0 * sin(tSlow * 0.33 + a01 * 2.0);
    vec2 kUV = reflectUV(tcRT, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(tSlow * 0.42 + a01 * 3.0);
    kUV = fractalFold(kUV, foldZoom, tFast, m, aspect);
    kUV = rotateUV(kUV, tSlow * 0.23 + a01 * 1.1, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(tSlow * 0.2) * (PI * tSlow), 5.0);
    float period = log(base) * pingPong(tSlow * PI, 5.0);
    float tz = tSlow * 0.65;
    float rD = diamondRadius(p) + 1e-6;

    float ang = atan(q.y, q.x) + tz * 0.35
              + 0.35 * sin(rD * 18.0 + tFast * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);

    vec2 dirK = normalize(pwrap + 1e-6);
    vec2 off = dirK * (0.0015 + 0.001 * sin(tFast * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((tcRT - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec2 sR = mix(u0, fR, 0.6);
    vec2 sG = mix(u1, fG, 0.6);
    vec2 sB = mix(u2, fB, 0.6);

    vec3 rC = preBlendColor(sR + off, tSlow, tFast, aspect);
    vec3 gC = preBlendColor(sG,       tSlow, tFast, aspect);
    vec3 bC = preBlendColor(sB - off, tSlow, tFast, aspect);

    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + tFast * 1.2));
    ring = ring * pingPong(tSlow * PI, 5.0);
    float pulse = 0.5 + 0.5 * sin(tFast * 2.0 + rD * 28.0 + k * 12.0);
    pulse *= mix(0.4, 1.4, a01);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.10
               + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.06;

    float pulseAdd = 0.004 * (0.5 + 0.5 * sin(tFast * 3.7 + seed));
    outCol += bloom;
    outCol += pulseAdd * ua;

    outCol = clamp(outCol, vec3(0.0), vec3(1.0));
    outCol = limitHighlights(outCol);

    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * (0.6 + 0.3 * a01));
    finalRGB = limitHighlights(finalRGB);
    finalRGB = clamp(finalRGB, 0.0, 1.0);

    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd6i_amp = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;

const float PI = 3.1415926535897932384626433832795;

float gAmp01;
float gSlow;
float gFast;
float gDetail;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    float f = mix(0.4, 2.0, gAmp01);
    p = sin(abs(p * f));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    float aAcc = clamp(amp, 0.0, 4.0);
    float aInst = clamp(uamp, 0.0, 4.0);
    float ampMix = clamp(aAcc * 0.6 + aInst * 1.4, 0.0, 4.0);
    gAmp01 = clamp(ampMix / 2.5, 0.0, 1.0);

    gSlow = time_f * mix(0.15, 0.7, gAmp01);
    gFast = time_f * mix(0.6, 3.5, gAmp01);
    gDetail = time_f * mix(0.3, 2.0, gAmp01);

    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * gSlow), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);

    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);

    vec3 baseCol = preBlendColor(TexCoord);

    float seg = 4.0 + 2.0 * sin(gSlow * 0.33 + gAmp01 * 2.0);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    float foldZoom = 1.45 + 0.55 * sin(pingPong(gSlow * PI, 25.0) * 0.42 + gAmp01 * 3.0);
    kUV = fractalFold(kUV, foldZoom, gDetail, m, aspect);
    kUV = rotateUV(kUV, gSlow * 0.23 + gAmp01 * 1.2, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(gSlow * 0.2) * (PI * gSlow), 5.0);
    float period = log(base) * pingPong(gSlow * PI, 5.0);
    float tz = gSlow * 0.65;
    float rD = diamondRadius(p) + 1e-6;

    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + gFast * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(gFast * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + gFast * 1.2));
    ring = ring * pingPong(gSlow * PI, 5.0);

    float pulse = 0.5 + 0.5 * sin(gFast * 2.0 + rD * 28.0 + k * 12.0);
    pulse *= mix(0.4, 1.4, gAmp01);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.10 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.08;
    outCol += bloom;

    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    outCol *= mix(0.7, 1.15, gAmp01);

    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    finalRGB = clamp(finalRGB, 0.0, 1.0);

    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd6i_bowl = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;
uniform float seed;

const float PI = 3.1415926535897932384626433832795;

float h1(float n){return fract(sin(n*91.345+37.12)*43758.5453123);}
vec2 h2(vec2 p){return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);}
vec2 rot(vec2 v,float a){float c=cos(a),s=sin(a);return vec2(c*v.x-s*v.y,s*v.x+c*v.y);}

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 cent, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - cent;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + cent;
}

vec2 reflectUV(vec2 uv, float segments, vec2 cent, float aspect) {
    vec2 p = uv - cent;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + cent;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 cent, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - cent) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + cent;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, cent, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 col) {
    col = pow(max(col, 0.0), vec3(0.95));
    float l = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(l), col, 0.9);
    return clamp(col, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 cent, float aspect) {
    vec2 p = (uv - cent) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + cent;
}

void main(void){
    float a = clamp(amp,0.0,1.0);
    float ua = clamp(uamp,0.0,1.0);
    float t = time_f;

    vec2 center = vec2(0.5, 0.5);
    vec2 baseUV = TexCoord;
    vec2 offset = baseUV - center;
    float maxRadius = length(vec2(0.5, 0.5));
    float radius = length(offset);
    float normalizedRadius = radius / maxRadius;

    float distortion = 0.25 + 0.45*ua + 0.3*a;
    float distortedRadius = normalizedRadius + distortion * normalizedRadius * normalizedRadius;
    distortedRadius = clamp(distortedRadius, 0.0, 1.0);
    distortedRadius *= maxRadius;
    vec2 normDir = radius > 0.0 ? offset / radius : vec2(0.0);
    vec2 distortedCoords = center + distortedRadius * normDir;

    float spinSpeed = 0.6 + 1.8*(0.3 + 0.7*a);
    float modulatedTime = pingPong(t * spinSpeed, 5.0);
    float angSpin = atan(distortedCoords.y - center.y, distortedCoords.x - center.x) + modulatedTime;

    vec2 rotatedTC;
    rotatedTC.x = cos(angSpin) * (distortedCoords.x - center.x) - sin(angSpin) * (distortedCoords.y - center.y) + center.x;
    rotatedTC.y = sin(angSpin) * (distortedCoords.x - center.x) + cos(angSpin) * (distortedCoords.y - center.y) + center.y;

    float warpAmp = 0.02 + 0.06*ua + 0.04*a;
    vec2 uvWarp;
    uvWarp.x = pingPong(rotatedTC.x + t * 0.12 * (1.0 + warpAmp*5.0), 1.0);
    uvWarp.y = pingPong(rotatedTC.y + t * 0.12 * (1.0 + warpAmp*5.0), 1.0);

    vec2 uv = uvWarp;

    float speedR=5.0, ampR=0.03, waveR=10.0;
    float speedG=6.5, ampG=0.025, waveG=12.0;
    float speedB=4.0, ampB=0.035, waveB=8.0;

    float rR=sin(uv.x*waveR+t*speedR)*ampR + sin(uv.y*waveR*0.8+t*speedR*1.2)*ampR;
    float rG=sin(uv.x*waveG*1.5+t*speedG)*ampG + sin(uv.y*waveG*0.3+t*speedG*0.7)*ampG;
    float rB=sin(uv.x*waveB*0.5+t*speedB)*ampB + sin(uv.y*waveB*1.7+t*speedB*1.3)*ampB;

    vec2 tcR=uv+vec2(rR,rR);
    vec2 tcG=uv+vec2(rG,-0.5*rG);
    vec2 tcB=uv+vec2(0.3*rB,rB);

    vec3 pats[4]=vec3[](vec3(1,0,1),vec3(0,1,0),vec3(1,0,0),vec3(0,0,1));
    float pspd=4.0;
    int pidx=int(mod(floor(t*pspd+seed*4.0),4.0));
    vec3 mir=pats[pidx];

    vec2 m = iMouse.z>0.5 ? (iMouse.xy/iResolution) : fract(vec2(0.37+0.11*sin(t*0.63+seed),0.42+0.13*cos(t*0.57+seed*2.0)));
    vec2 dR=tcR-m, dG=tcG-m, dB=tcB-m;

    float fallR=smoothstep(0.55,0.0,length(dR));
    float fallG=smoothstep(0.55,0.0,length(dG));
    float fallB=smoothstep(0.55,0.0,length(dB));

    float sw=(0.12+0.38*ua+0.25*a);
    vec2 tangR=rot(normalize(dR+1e-4),1.5707963);
    vec2 tangG=rot(normalize(dG+1e-4),1.5707963);
    vec2 tangB=rot(normalize(dB+1e-4),1.5707963);

    vec2 airR=tangR*sw*fallR*(0.06+0.22*a)*(0.6+0.4*cos(uv.y*40.0+t*3.0+seed));
    vec2 airG=tangG*sw*fallG*(0.06+0.22*a)*(0.6+0.4*cos(uv.y*38.0+t*3.3+seed*1.7));
    vec2 airB=tangB*sw*fallB*(0.06+0.22*a)*(0.6+0.4*cos(uv.y*42.0+t*2.9+seed*0.9));

    vec2 jit = (h2(uv*vec2(233.3,341.9)+t+seed)-0.5)*(0.0006+0.004*ua);
    tcR += airR + jit;
    tcG += airG + jit;
    tcB += airB + jit;

    vec2 fR=vec2(mir.r>0.5?1.0-tcR.x:tcR.x, tcR.y);
    vec2 fG=vec2(mir.g>0.5?1.0-tcG.x:tcG.x, tcG.y);
    vec2 fB=vec2(mir.b>0.5?1.0-tcB.x:tcB.x, tcB.y);

    float ca=0.0015+0.004*a;
    vec4 C=texture(textTexture,uv);
    C.r=texture(textTexture,fR+vec2( ca,0)).r;
    C.g=texture(textTexture,fG              ).g;
    C.b=texture(textTexture,fB+vec2(-ca,0)).b;

    float pulseChrom=0.004*(0.5+0.5*sin(t*3.7+seed));
    C.rgb+=pulseChrom*ua;
    vec3 chromRGB = C.rgb;

    vec4 baseTex = texture(textTexture, uv);
    vec2 uvN = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uvN.x *= aspect;
    float r = pingPong(sin(length(uvN) * time_f), 5.0);
    float radiusN = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radiusN, radiusN - 0.25, r);

    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(uv);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(uv, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float baseZ = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(baseZ) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;

    float mixNeonBase = pingPong(pulse * PI, 5.0) * 0.18;
    vec3 neonBaseMix = mix(baseCol, outCol, 0.65 + 0.35*ua);
    vec3 neonOut = mix(baseTex.rgb, neonBaseMix, mixNeonBase + 0.45);
    neonOut = clamp(neonOut, vec3(0.05), vec3(0.97));

    float glowMix = pingPong(glow * PI, 5.0) * 0.8;
    vec3 neonFinal = mix(baseTex.rgb, neonOut, glowMix);

    float layerMix = clamp(0.35 + 0.4*a + 0.25*ua, 0.0, 1.0);
    vec3 finalRGB = mix(neonFinal, chromRGB, layerMix);

    color = vec4(finalRGB, 1.0);
}
)SHD";
inline const char *src_frac_shader02_dmd6i_bubble = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    float audio = clamp(amp * 0.8 + uamp * 0.6, 0.0, 5.0);
    float audioNorm = clamp(audio * 0.5, 0.0, 2.5);
    float t = time_f * (1.0 + audioNorm * 0.25);

    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);

    vec2 rel = TexCoord - m;
    float dist = length(rel);
    float bubSize = 0.38 + 0.12 * audioNorm;
    float bubEdge = 0.28 + 0.10 * audioNorm;
    float bubMask = 1.0 - smoothstep(bubSize, bubSize + bubEdge, dist);

    float zoomBase = 1.3 + 0.7 * audioNorm;
    float zoomAnim = 0.5 + 0.5 * sin(t * 1.6 + audioNorm * 2.3);
    float zoom = mix(1.0, zoomBase, zoomAnim);

    vec2 tcZoom = m + rel / zoom;
    vec2 tcF = mix(TexCoord, tcZoom, bubMask);

    vec4 baseTex = texture(textTexture, tcF);

    vec2 uv = tcF * 2.0 - 1.0;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * t), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);

    vec3 baseCol = preBlendColor(tcF);

    float seg = 4.0 + 2.0 * sin(t * 0.33);
    vec2 kUV = reflectUV(tcF, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    float foldZoom = 1.45 + 0.55 * sin(t * 0.42);
    kUV = fractalFold(kUV, foldZoom, t, m, aspect);
    kUV = rotateUV(kUV, t * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(t * 0.2) * (PI * t), 5.0);
    float period = log(base) * pingPong(t * PI, 5.0);
    float tz = t * 0.65;

    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + t * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);

    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(t * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((tcF - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float angDir = atan(p.y, p.x) / (2.0 * PI);
    angDir = angDir * 0.5 + 0.5;
    float radN = clamp(length(p) * 0.9, 0.0, 1.0);

    float hueShift = pingPong(time_f * 0.18 + audioNorm * 0.12, 1.0);
    float hue = fract(angDir + (radN - 0.5) * 0.4 + (hueShift - 0.5) * 0.6);
    float sat = 0.4 + 0.4 * radN;
    float val = 0.45 + 0.45 * (1.0 - radN);

    vec3 gradCol = hsv2rgb(vec3(hue, sat, val));

    float gMix = (0.18 + 0.25 * audioNorm) * (0.3 + 0.7 * bubMask);
    vec3 outCol = mix(kaleidoRGB, kaleidoRGB * gradCol, gMix);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + t * 1.2));
    ring = ring * pingPong((t * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(t * 2.0 + rD * 28.0 + k * 12.0 + audioNorm * 3.0);

    outCol *= (0.60 + 0.20 * ring) * (0.80 + 0.10 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.08;
    outCol += bloom * 0.3;

    outCol = mix(outCol, baseTex.rgb, 0.18 + audioNorm * 0.04);

    float rim = smoothstep(bubSize + 0.02, bubSize - 0.02, dist);
    vec3 bubbleHighlight = hsv2rgb(vec3(hue, 0.7, 0.9)) * rim * (0.10 + 0.20 * audioNorm);
    outCol += bubbleHighlight * bubMask * 0.5;

    outCol = clamp(outCol, vec3(0.05), vec3(0.90));

    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.7 + 0.1);

    float maxC = max(max(finalRGB.r, finalRGB.g), finalRGB.b);
    float targetMax = 0.97;
    if (maxC > targetMax) finalRGB *= targetMax / maxC;
    finalRGB = clamp(finalRGB, vec3(0.03), vec3(0.97));

    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd6i_neon = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec3 origRGB = baseTex.rgb;

    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;

    float r = pingPong(sin(length(uv) * time_f), 5.0); 
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);

    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);

    vec3 baseCol = preBlendColor(TexCoord);

    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;

    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));

    vec3 finalRGB = mix(origRGB, outCol, pingPong(glow * PI, 5.0) * 0.8);

    float lum = dot(finalRGB, vec3(0.299, 0.587, 0.114));
    float audio = clamp(amp * 1.5 + uamp * 0.35, 0.0, 2.0);
    float tHue = time_f * 0.15 + uamp * 0.12;
    float hueBase = fract(lum * 0.8 + tHue);
    vec3 neon1 = hsv2rgb(vec3(hueBase, 1.0, 1.0));
    vec3 neon2 = hsv2rgb(vec3(fract(hueBase + 0.33), 1.0, 1.0));
    float wave = pingPong(time_f * 0.25 + amp * 2.0, 1.0);
    vec3 neon = mix(neon1, neon2, wave);

    float strength = clamp(0.2 + audio * 0.6, 0.0, 1.0);
    vec3 texNeon = mix(finalRGB, neon, strength);

    vec3 intertwined = mix(texNeon * origRGB, texNeon, 0.4 + 0.5 * clamp(audio, 0.0, 1.0));

    vec3 mixed = pow(intertwined, vec3(0.8));
    mixed = clamp(mixed, 0.0, 1.0);

    color = vec4(mixed, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd6i1 = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0); 
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * (PI/2.0)), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = vec3(kaleidoRGB.r * pingPong(time_f * PI, 2.0), kaleidoRGB.g * pingPong(time_f * PI, 2.0), kaleidoRGB.b * pingPong(time_f * PI, 2.0));
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;

    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse *  PI*2.0, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd7i = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p * (pingPong(time_f * PI, 5.0))));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0); 
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;

    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse *  PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd8i = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p * (pingPong(time_f * PI, 5.0))));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0); 
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(pingPong(time_f * PI, 25.0) * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;

    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse *  PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmd9i = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / sin(segments * pingPong(time_f, 5.0));
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (sin(pingPong(zoom * time_f * PI, 5.0)) + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p * (pingPong(time_f * PI, 5.0))));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec4 baseTex = texture(textTexture, TexCoord);
    vec2 uv = TexCoord * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0); 
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(TexCoord);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(TexCoord, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(pingPong(time_f * PI, 25.0) * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((TexCoord - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;

    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse *  PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmdi_radial = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
uniform float amp;
uniform float uamp;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    float aspect = iResolution.x / iResolution.y;
    vec2 frag = gl_FragCoord.xy / iResolution;
    vec2 normCoord = (frag * 2.0 - 1.0) * vec2(aspect, 1.0);
    float distanceFromCenter = length(normCoord);
    float wave = sin(distanceFromCenter * 12.0 - time_f * 4.0);
    float aNorm = clamp(amp, 0.0, 1.5);
    float uNorm = clamp(uamp, 0.0, 1.5);
    float waveStrength = mix(0.06, 0.30, clamp(uNorm * 0.7, 0.0, 1.0));
    waveStrength *= 0.8 + 0.4 * aNorm;
    vec2 waveOffset = normCoord * waveStrength * wave;
    vec2 tcWave = TexCoord + waveOffset;

    vec4 baseTex = texture(textTexture, tcWave);

    vec2 uv = tcWave * 2.0 - 1.0;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);

    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);

    vec3 baseCol = preBlendColor(tcWave);

    float seg = 4.0 + 2.0 * sin(time_f * 0.33 + uNorm * 0.5);
    vec2 kUV = reflectUV(tcWave, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.35 + 0.65 * sin(time_f * 0.42 + uNorm * 0.8);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * (0.18 + 0.10 * aNorm), m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.75 + 0.25 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * (0.55 + 0.25 * uNorm);
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * (1.03 + 0.02 * uNorm)) / ar + m);
    vec2 u2 = fract((pwrap * (0.97 - 0.02 * uNorm)) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0010 + 0.0015 * uNorm) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((tcWave - m) * ar));
    vign = mix(0.9, 1.18, vign);

    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring *= pingPong(time_f * PI + uNorm * 2.5, 5.0);

    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0 + aNorm * 1.5);

    vec3 outCol = kaleidoRGB;
    float gain = 0.7 + 0.3 * uNorm;
    outCol *= (0.75 + 0.25 * ring * gain) * (0.85 + 0.15 * pulse * gain) * vign;

    vec3 bloom = outCol * outCol * (0.04 + 0.12 * uNorm) +
                 pow(max(outCol - 0.6, 0.0), vec3(2.0)) * (0.04 + 0.08 * uNorm);

    outCol += bloom;

    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * (0.15 + 0.15 * (1.0 - uNorm * 0.5)));
    outCol = clamp(outCol, vec3(0.04), vec3(0.96));

    float mixK = pingPong(glow * PI, 5.0) * (0.45 + 0.35 * uNorm);
    vec3 finalRGB = mix(baseTex.rgb, outCol, mixK);
    finalRGB = clamp(finalRGB, vec3(0.03), vec3(0.97));

    color = vec4(finalRGB, baseTex.a);
}
)SHD";
inline const char *src_frac_shader02_dmdi6i_zoom = R"SHD(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
)SHD" COMMON_UNIFORMS COLOR_HELPERS R"SHD(
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    float aspect = iResolution.x / iResolution.y;
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);

    float zoomPhase = time_f * 0.12;
    float zCycle = floor(zoomPhase);
    float zLocal = fract(zoomPhase);
    float tri = 1.0 - abs(zLocal * 2.0 - 1.0);
    float sgn = mix(-1.0, 1.0, step(0.5, mod(zCycle, 2.0)));
    float depth = 1.0 + zCycle * 0.35;
    float zoomExp = sgn * tri * depth;
    float zoom = pow(1.45, zoomExp);

    vec2 z = TexCoord - m;
    z.x *= aspect;
    z /= zoom;
    z.x /= aspect;
    vec2 zoomTC = fract(z + m);

    vec4 baseTex = texture(textTexture, zoomTC);

    vec2 uv = zoomTC * 2.0 - 1.0;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);

    vec3 baseCol = preBlendColor(zoomTC);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(zoomTC, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((zoomTC - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;

    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;

    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);

    color = vec4(finalRGB, baseTex.a);
}
)SHD";


inline const char *color_shader02 = R"SHD(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
uniform float amp;
uniform float uamp;

const float PI = 3.14159265359;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}


float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}


float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    mat2 m = mat2(1.6, -1.2, 1.2, 1.6);

    for (int i = 0; i < 6; i++) {
        v += a * noise(p);
        p = m * p;
        a *= 0.5;
    }
    return v;
}


vec2 warp(vec2 p, float t) {
    float n1 = fbm(p + t * 0.15);
    float n2 = fbm(p + vec2(4.1, 7.3) - t * 0.12);
    return p + vec2(n1, n2) * 0.8;
}

float pingPong(float x, float length){
    float m = mod(x, length*2.0);
    return m <= length ? m : length*2.0 - m;
}


void main(void) {
    vec2 tc = TexCoord;
    vec4 tex = texture(textTexture, tc);

    float aspect = iResolution.x / max(iResolution.y, 1.0);
    vec2 uv = tc * 2.0 - 1.0;
    uv.x *= aspect;

    float t = time_f;
    float aamp = clamp(abs(amp) * 0.8 + abs(uamp) * 0.2, 0.0, 1.0);

    vec2 p = uv * (0.8 + 0.6 * sin(t * 0.07));
    p = warp(p * 1.2, t);

    float d1 = fbm(p * 1.4 + t * 0.05);
    float d2 = fbm(p * 2.6 - t * 0.03);
    float density = mix(d1, d2, 0.5);
    density = smoothstep(0.2, 0.85, density);

    float hue = fract(
        0.10 * t +
        0.35 * density +
        0.15 * sin(p.x * 1.4 + t * 0.4) +
        0.15 * sin(p.y * 1.7 - t * 0.3)
    );

    float sat = 0.85 + 0.15 * sin(t * 0.3 + density * 3.0);
    float val = 0.6 + 0.8 * density;

    vec3 cloudColor = hsv2rgb(vec3(hue, sat, val));

    vec3 glow = cloudColor * cloudColor * (0.6 + 0.6 * density);
    cloudColor += glow * 0.8;

    cloudColor += 0.08 * sin(vec3(1.3, 2.1, 3.2) * (density * 4.0 + t));

    float vign = 1.0 - smoothstep(0.7, 1.3, length(uv));
    cloudColor *= mix(0.85, 1.15, vign);

    float texLum = dot(tex.rgb, vec3(0.299, 0.587, 0.114));
    vec3 darkTex = mix(tex.rgb * 0.65, vec3(texLum) * 0.55, 0.35);

    float mixAmt = 0.3;
    vec3 finalRGB = mix(darkTex, cloudColor, mixAmt);

    finalRGB = pow(max(finalRGB, 0.0), vec3(0.85));
    finalRGB = clamp(finalRGB, 0.0, 1.0);

    color = vec4(finalRGB, tex.a);
})SHD";


#endif