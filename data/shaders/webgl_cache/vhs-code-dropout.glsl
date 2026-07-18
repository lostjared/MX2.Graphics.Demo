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
    p = fract(p * vec2(443.897, 441.423));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y);
}

void mxCacheShaderMain() {
    vec2 uv = tc;
    float frame = floor(time_f * 24.0);
    float line = floor(uv.y * max(iResolution.y, 1.0));
    float block = floor(uv.x * 28.0);
    float segmentNoise = hash21(vec2(block + frame * 0.17, line));
    float damagedLine = step(0.965, hash21(vec2(line, floor(frame * 0.5))));
    float dropout = damagedLine * step(0.42, segmentNoise);

    float lineShift = (hash21(vec2(line, frame)) - 0.5) * damagedLine * 0.075;
    uv.x += lineShift + sin(uv.y * 190.0 + time_f * 2.7) * 0.0012;
    uv = clamp(uv, 0.001, 0.999);

    vec3 image = texture(samp, uv).rgb;
    vec3 separated;
    separated.r = texture(samp, uv + vec2(0.0025, 0.0)).r;
    separated.g = image.g;
    separated.b = texture(samp, uv - vec2(0.003, 0.0)).b;
    image = mix(image, separated, 0.65);

    float grain = hash21(gl_FragCoord.xy + frame * 13.0) - 0.5;
    vec3 dropoutColor = vec3(0.62 + grain * 0.7);
    image = mix(image, dropoutColor, dropout * 0.78);
    image += grain * 0.035;
    image *= 0.96 + 0.04 * sin(line * 3.14159265);

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
