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
uniform sampler2D samp;
uniform vec2 iResolution;

float h1(float n){return fract(sin(n)*43758.5453123);}
vec2 h2(float n){return fract(sin(vec2(n,n+1.0))*vec2(43758.5453,22578.1459));}

void mxCacheShaderMain() {
    float t = time_f;
    vec2 uv = tc;

    float rate = 0.35;
    float k = floor(t*rate);
    float a = fract(t*rate);
    a = a*a*(3.0-2.0*a);
    vec2 p0 = h2(k);
    vec2 p1 = h2(k+1.0);
    vec2 shift = (mix(p0,p1,a)-0.5)*0.08;

    vec2 c = vec2(0.5)+shift;
    vec2 p = uv - c;
    float r2 = dot(p, p);
    p += p * r2 * (0.035 + 0.01*h1(k+7.0)) * sin(t * (0.4 + 0.05*h1(k+13.0)));
    uv = p + c;

    vec2 d1 = vec2(sin(uv.y * 12.0 - t * 2.0), cos(uv.x * 12.0 + t * 1.6)) * 0.015;
    vec2 d2 = vec2(sin((uv.x + uv.y) * 24.0 + t * 1.2), -cos((uv.x - uv.y) * 24.0 - t * 1.8)) * 0.009;
    vec2 d3 = vec2(cos(uv.y * 40.0 + t * 3.5), sin(uv.x * 40.0 - t * 3.0)) * 0.003;

    uv += d1 + d2 + d3;
    uv = clamp(uv, 0.0, 1.0);

    color = texture(samp, uv);
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
