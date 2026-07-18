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
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

void mxCacheShaderMain() {
    vec2 uv = tc;
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 d = uv - m;
    float dist = length(d);
    float pos = 1.0 + 6.0 * (1.0 - smoothstep(0.0, 0.35, dist)) + 0.25 * sin(time_f * 1.5);

    // warp coordinates based on mouse influence
    vec2 warp = d * sin(time_f * 0.8 + dist * 8.0) * 0.15 * pos;
    uv += warp;

    ivec2 coords = ivec2(uv * iResolution);
    vec4 origColor = texture(samp, uv);

    int x = coords.x;
    int y = coords.y;
    vec3 newColor = origColor.rgb;

    if ((x % 2) == 0) {
        if ((y % 2) == 0) {
            newColor.r = (1.0 - pos * origColor.r);
            newColor.b = (float(x + y) * pos) / 255.0;
        } else {
            newColor.r = (pos * origColor.r - float(y)) / 255.0;
            newColor.b = (float(x - y) * pos) / 255.0;
        }
    } else {
        if ((y % 2) == 0) {
            newColor.r = (pos * origColor.r - float(x)) / 255.0;
            newColor.b = (float(x - y) * pos) / 255.0;
        } else {
            newColor.r = (pos * origColor.r - float(y)) / 255.0;
            newColor.b = (float(x + y) * pos) / 255.0;
        }
    }

    float temp = newColor.r;
    newColor.r = newColor.b;
    newColor.b = temp;
    vec3 finalColor = (sin(time_f) > 0.0) ? vec3(1.0) - newColor : newColor;

    color = sin(vec4(finalColor, 1.0) * pingPong(time_f, 10.0));
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
