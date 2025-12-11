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


#endif