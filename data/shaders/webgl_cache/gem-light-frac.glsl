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
uniform vec2 iResolution;
uniform float time_f;

// Helper for smoother color cycling
vec3 palette(float t) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263, 0.416, 0.557);
    return a + b * cos(6.28318 * (c * t + d));
}

void mxCacheShaderMain() {
    // 1. Setup coordinates (centered and aspect-corrected)
    vec2 uv = (tc * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
    vec2 uv0 = uv; // Store original UVs for global glow
    vec3 finalColor = vec3(0.0);

    // 2. The Fractal Loop
    // This is where the magic happens. We iterate to create layers.
    for (float i = 0.0; i < 4.0; i++) {

        // FRACTAL FOLDING:
        // This 'fract' creates the repetition. The '-0.5' re-centers each "tile".
        uv = fract(uv * 1.5) - 0.5;

        // Calculate distance from center of the current fold
        float d = length(uv) * exp(-length(uv0));

        // Create a pulsing spectral color based on depth (i) and time
        vec3 col = palette(length(uv0) + i * 0.4 + time_f * 0.4);

        // This equation creates the "neon" thin lines
        // d starts as distance, sin makes it a wave, abs makes it a sharp line
        d = sin(d * 8.0 + time_f) / 8.0;
        d = abs(d);

        // Intensify the glow (inverse relationship)
        d = pow(0.01 / d, 1.2);

        finalColor += col * d;
    }

    // 3. Texture Integration
    // We distort the texture lookup using the fractal coordinates
    vec2 distortion = uv * 0.1;
    vec4 texColor = texture(samp, tc + distortion);

    // Mix the fractal neon with the texture
    vec3 composite = mix(texColor.rgb, finalColor, 0.6);

    // Add a bit of your original "sin" madness for the finish
    color = vec4(composite, texColor.a);
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
