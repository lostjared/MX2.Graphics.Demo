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
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

vec3 sampleSharp(vec2 uv) {
    vec2 pixel = 1.0 / max(iResolution, vec2(1.0));
    vec3 center = texture(samp, uv).rgb;
    vec3 blur = texture(samp, clamp(uv + vec2(pixel.x, 0.0), 0.0, 1.0)).rgb;
    blur += texture(samp, clamp(uv - vec2(pixel.x, 0.0), 0.0, 1.0)).rgb;
    blur += texture(samp, clamp(uv + vec2(0.0, pixel.y), 0.0, 1.0)).rgb;
    blur += texture(samp, clamp(uv - vec2(0.0, pixel.y), 0.0, 1.0)).rgb;
    blur *= 0.25;
    return clamp(center + (center - blur) * 0.65, 0.0, 1.0);
}

vec3 neonPalette(vec3 source) {
    float luma = dot(source, vec3(0.299, 0.587, 0.114));
    float warmth = source.r - source.b;
    float coolness = source.b - source.r;

    vec3 graded = source;
    graded.r += warmth * 0.18 + source.b * 0.08;
    graded.g += coolness * 0.14;
    graded.b += coolness * 0.24 + source.r * 0.04;

    vec3 cyanShadow = vec3(0.015, 0.10, 0.14);
    vec3 magentaLight = vec3(1.0, 0.16, 0.52);
    float shadowMask = 1.0 - smoothstep(0.08, 0.48, luma);
    float highlightMask = smoothstep(0.48, 0.98, luma);
    graded = mix(graded, cyanShadow + graded * 0.58, shadowMask * 0.72);
    graded = mix(graded, magentaLight + graded * 0.42, highlightMask * 0.34);

    luma = dot(graded, vec3(0.299, 0.587, 0.114));
    graded = mix(vec3(luma), graded, 1.30);
    graded = (graded - 0.5) * 1.20 + 0.5;
    return clamp(graded, 0.0, 1.0);
}

void mxCacheShaderMain() {
    vec2 uv = tc;
    vec2 pixel = 1.0 / max(iResolution, vec2(1.0));
    float frame = floor(time_f * 29.97);
    float line = floor(uv.y * max(iResolution.y, 1.0));

    float lineWobble = sin(uv.y * 170.0 + time_f * 2.4) * pixel.x * 1.4;
    float trackingNoise = hash21(vec2(line, frame)) - 0.5;
    float trackingBand = 1.0 - smoothstep(0.0, 0.035, abs(uv.y - fract(time_f * 0.075)));
    uv.x += lineWobble + trackingNoise * pixel.x * 0.7;
    uv.x += trackingBand * trackingNoise * 0.055;
    uv = clamp(uv, pixel, vec2(1.0) - pixel);

    float chromaShift = pixel.x * 2.2 + trackingBand * 0.003;
    vec3 source = sampleSharp(uv);
    source.r = sampleSharp(clamp(uv + vec2(chromaShift, 0.0), 0.0, 1.0)).r;
    source.b = sampleSharp(clamp(uv - vec2(chromaShift * 1.35, 0.0), 0.0, 1.0)).b;
    vec3 image = neonPalette(source);

    vec3 glow = texture(samp, clamp(uv + pixel * vec2(4.0, 2.0), 0.0, 1.0)).rgb;
    glow += texture(samp, clamp(uv - pixel * vec2(4.0, 2.0), 0.0, 1.0)).rgb;
    glow += texture(samp, clamp(uv + pixel * vec2(-4.0, 2.0), 0.0, 1.0)).rgb;
    glow += texture(samp, clamp(uv + pixel * vec2(4.0, -2.0), 0.0, 1.0)).rgb;
    glow *= 0.25;
    glow = neonPalette(glow);
    image += max(glow - 0.64, 0.0) * vec3(0.85, 0.42, 1.10) * 0.48;

    float scanline = 0.965 + 0.035 * sin(line * 3.14159265);
    float grain = hash21(gl_FragCoord.xy + frame * 17.0) - 0.5;
    float flicker = 0.985 + hash21(vec2(frame, 4.0)) * 0.015;
    float vignette = 1.0 - smoothstep(0.22, 0.78, length((tc - 0.5) * vec2(1.08, 1.0)));
    image *= scanline * flicker;
    image += grain * (0.022 + trackingBand * 0.12);
    image *= mix(0.80, 1.0, vignette);

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
