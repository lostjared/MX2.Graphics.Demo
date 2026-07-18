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

float hash(vec2 p){ return fract(sin(dot(p, vec2(127.1,311.7))) * 43758.5453); }

vec3 compositeEffect(vec2 uv) {
    vec2 c = vec2(0.5);
    vec2 d = uv - c;
    float r2 = dot(d,d);
    vec2 uvd = c + d * (1.0 + 0.15*r2 + 0.15*r2*r2);
    float wob = (hash(vec2(time_f*0.73, uv.y*13.7)) - 0.5) * 0.003;
    uvd.x += wob;
    uvd = clamp(uvd, 0.001, 0.999);

    float ca = 0.002 + 0.002*sin(time_f*0.37);
    vec2 shift = d * ca;
    float bleed = sin(uvd.y * iResolution.y * 0.2 + time_f * 5.0) * 0.003;

    float off = 0.01;
    vec3 col;
    col.r = texture(samp, uvd + vec2(off + bleed, 0.0) + shift).r;
    col.g = texture(samp, uvd).g;
    col.b = texture(samp, uvd - vec2(off + bleed, 0.0) - shift).b;

    float grain = hash(uvd * iResolution.xy + time_f*41.0) * 2.0 - 1.0;
    col += grain * 0.03;

    float scan = sin(uvd.y * iResolution.y * 3.14159) * 0.06;
    col -= scan;

    float l = dot(col, vec3(0.2126,0.7152,0.0722));
    col = mix(vec3(l), col, 0.92);

    float vig = 1.0 - smoothstep(0.25, 0.8, length(d));
    col *= vig;

    col = pow(max(col, 0.0), vec3(1.0/1.7));
    return clamp(col, 0.0, 1.0);
}

void mxCacheShaderMain() {
    vec3 col = compositeEffect(tc);
    color = vec4(col, 1.0);
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
