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
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void mxCacheShaderMain() {
    vec2 uv = tc;
    float frame = floor(time_f * 29.97);
    float line = floor(uv.y * max(iResolution.y, 1.0));
    float handheld = sin(time_f * 2.3) * 0.0015 + sin(time_f * 5.7) * 0.0008;
    uv.x += handheld + sin(uv.y * 145.0 + time_f * 2.2) * 0.0012;
    uv.y += (hash21(vec2(frame, 6.0)) - 0.5) * 0.0025;
    uv = clamp(uv, 0.002, 0.998);

    vec3 image;
    image.r = texture(samp, uv + vec2(0.0022, 0.0)).r;
    image.g = texture(samp, uv).g;
    image.b = texture(samp, uv - vec2(0.0032, 0.0)).b;

    float luma = dot(image, vec3(0.299, 0.587, 0.114));
    image = mix(vec3(luma), image, 0.82);
    image *= vec3(1.08, 0.99, 0.84);
    image = (image - 0.5) * 0.92 + 0.54;

    float grain = hash21(gl_FragCoord.xy + frame * 11.0) - 0.5;
    float scan = sin(line * 3.14159265);
    float vignette = 1.0 - smoothstep(0.25, 0.78, length((tc - 0.5) * vec2(1.15, 1.0)));
    float exposureFlicker = 0.96 + 0.04 * hash21(vec2(floor(frame * 0.5), 3.0));
    image = image * exposureFlicker + grain * 0.035;
    image *= 0.96 + scan * 0.025;
    image *= mix(0.76, 1.0, vignette);

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
