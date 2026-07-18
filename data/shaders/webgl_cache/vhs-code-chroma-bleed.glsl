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

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void mxCacheShaderMain() {
    vec2 uv = tc;
    float frame = floor(time_f * 30.0);
    float line = floor(uv.y * max(iResolution.y, 1.0));
    float lineDrift = (hash21(vec2(line, frame)) - 0.5) * 0.002;
    float weave = sin(uv.y * 210.0 + time_f * 3.5) * 0.0015;
    uv.x += lineDrift + weave;

    vec2 chromaOffset = vec2(0.005 + 0.002 * sin(time_f * 0.7), 0.0);
    vec3 center = texture(samp, clamp(uv, 0.0, 1.0)).rgb;
    vec3 left = texture(samp, clamp(uv - chromaOffset, 0.0, 1.0)).rgb;
    vec3 right = texture(samp, clamp(uv + chromaOffset, 0.0, 1.0)).rgb;

    float luma = dot(center, vec3(0.299, 0.587, 0.114));
    float redChroma = right.r - dot(right, vec3(0.299, 0.587, 0.114));
    float blueChroma = left.b - dot(left, vec3(0.299, 0.587, 0.114));
    vec3 image = vec3(luma + redChroma, luma - 0.35 * redChroma - 0.25 * blueChroma,
                      luma + blueChroma);

    vec3 echoColor = texture(samp, clamp(uv - vec2(0.012, 0.0), 0.0, 1.0)).rgb;
    image = mix(image, echoColor, 0.08);
    float noise = hash21(gl_FragCoord.xy + frame * 9.0) - 0.5;
    image += noise * 0.025;
    image *= 0.96 + 0.04 * sin(uv.y * iResolution.y * 3.14159265);

    color = vec4(clamp(image, 0.0, 1.0), 1.0);
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
