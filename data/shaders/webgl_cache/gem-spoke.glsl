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

void mxCacheShaderMain() {
    // 1. Center and normalize coordinates
    vec2 uv = (tc * 2.0 - 1.0);
    uv.x *= iResolution.x / iResolution.y; // Maintain aspect ratio

    // 2. Fractal parameters
    float zoom = sin(time_f * 0.2) * 0.5 + 1.5;
    vec3 fractalCol = vec3(0.0);
    float scale = 1.0;

    // 3. The Iterative Fold (The "Fractal" part)
    // We loop to create layers of self-similarity
    for (int i = 0; i < 6; i++) {
        // Folding space: mirroring the UVs creates symmetrical complexity
        uv = abs(uv) - 0.5;

        // Rotation over time to create a "Kaleidoscope" effect
        float a = time_f * 0.3;
        float s = sin(a), c = cos(a);
        uv *= mat2(c, -s, s, c);

        // Scaling space inward
        uv *= 1.2;
        scale *= 1.2;

        // Calculate a "distance" based on the warped coordinates
        float d = length(uv);

        // Add glowing edges based on your original 'spoke' logic
        float glow = 0.01 / abs(sin(d * 8.0 - time_f) / 8.0);
        fractalCol += glow * vec3(0.2, 0.5, 1.0) / scale;
    }

    // 4. Texture Mapping
    // Use the warped UVs to sample the texture for a "infinite mirror" look
    vec4 texColor = texture(samp, fract(uv + time_f * 0.1));

    // 5. Combine original glow logic with fractal structure
    vec3 finalRGB = texColor.rgb * fractalCol;

    // Subtle vignette
    finalRGB *= smoothstep(1.5, 0.5, length(tc * 2.0 - 1.0));

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
