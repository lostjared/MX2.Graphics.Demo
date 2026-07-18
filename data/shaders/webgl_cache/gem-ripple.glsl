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

// Helper for seamless coordinate wrapping
vec2 mirror(vec2 uv) {
    vec2 m = mod(uv, 2.0);
    return mix(m, 2.0 - m, step(1.0, m));
}

// Better random vector generation
vec2 hash2(float n) {
    return fract(sin(vec2(n, n + 1.0)) * vec4(43758.5453, 12345.6789, 22578.1459, 98765.4321).xy);
}

void mxCacheShaderMain() {
    // 1. Smooth Path Generation
    float t = time_f * 0.5;
    float t_floor = floor(t);
    float t_fract = smoothstep(0.0, 1.0, fract(t)); // Smooth transition

    vec2 p0 = hash2(t_floor);
    vec2 p1 = hash2(t_floor + 1.0);
    vec2 center = mix(p0, p1, t_fract);

    // 2. Space Distortion
    vec2 uv = tc - center;
    float r = length(uv);
    float angle = atan(uv.y, uv.x);

    // Dynamic Swirl that breathes
    float swirl = sin(time_f * 0.4) * 3.0;
    angle += swirl * exp(-r * 2.0); // Swirl is strongest at the center

    // Reconstruct UVs
    vec2 warpedUV = center + vec2(cos(angle), sin(angle)) * r;

    // Add a gentle "liquid" wave
    warpedUV += 0.015 * vec2(sin(tc.y * 10.0 + time_f), cos(tc.x * 10.0 + time_f));

    // 3. Chromatic Aberration (The "Neat" Factor)
    // We sample the texture 3 times at slightly different offsets
    float shift = 0.005 * r;
    float r_chan = texture(samp, mirror(warpedUV + shift)).r;
    float g_chan = texture(samp, mirror(warpedUV)).g;
    float b_chan = texture(samp, mirror(warpedUV - shift)).b;

    // 4. Final Polish
    vec3 finalCol = vec3(r_chan, g_chan, b_chan);

    // Add a subtle vignette to focus on the center distortion
    float vignette = smoothstep(1.2, 0.2, r);
    finalCol *= vignette;

    color = vec4(finalCol, 1.0);
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
