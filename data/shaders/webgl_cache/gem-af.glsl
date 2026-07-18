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


vec2 mirror(vec2 uv) {
    return abs(fract(uv * 0.5 + 0.5) * 2.0 - 1.0);
}

void mxCacheShaderMain() {

    vec2 uv = (tc - 0.5) * 2.0;
    uv.x *= iResolution.x / iResolution.y;
    float iter = 0.0;
    float max_iter = 6.0;
    float time_pulse = sin(time_f * 0.2) * 0.5 + 0.5;

    for (float i = 0.0; i < max_iter; i++) {
        uv = abs(uv) - 0.5;

        float angle = time_f * 0.1 + i * 0.5;
        float s = sin(angle), c = cos(angle);
        uv *= mat2(c, -s, s, c);

        // Scale space: This creates the "infinite zoom" feel
        uv *= 1.1 + 0.1 * time_pulse;
        iter = i;
    }

    // 3. Texture Sampling with Fractal Distortion
    // We use the final warped UV to sample the texture
    vec2 fractal_tc = mirror(uv);
    vec4 texColor = texture(samp, fractal_tc);

    // 4. Color Logic (Evolved from your colorShift)
    // We use the iteration depth 'iter' to modulate colors
    vec3 fractalCol = 0.5 + 0.5 * cos(vec3(0.0, 2.0, 4.0) + length(uv) + time_f);

    // Mix the original texture with the mathematical fractal glow
    vec3 finalRGB = mix(texColor.rgb, fractalCol, 0.4);

    // Add a subtle "bloom" effect based on the fractal edges
    float bloom = 0.02 / abs(sin(length(uv) - time_f));
    finalRGB += bloom * fractalCol;

    color = vec4(finalRGB, 1.0);
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
