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

// Uniforms
uniform float time_f;
uniform sampler2D samp;
uniform vec2 iResolution;

// --------------------------------------------------------
// Random number generator (hash-based, deterministic)
// Generates values in the range [0.0, 1.0]
// --------------------------------------------------------
float rand(vec2 pos) {
    return fract(sin(dot(pos, vec2(12.9898, 78.233))) * 43758.5453);
}

// --------------------------------------------------------
// Draw Julia fractal at a given position with an animated seed
// --------------------------------------------------------
vec4 drawRandomJulia(vec2 uv, vec2 center, vec2 seed) {
    uv = (uv - center) * 3.0;  // Zoom in and center
    vec2 z = uv;
    const int MAX_ITER = 100;
    float brightness = 0.0;

    for (int i = 0; i < MAX_ITER; i++) {
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + seed;
        if (dot(z, z) > 4.0) {
            brightness = float(i) / float(MAX_ITER);
            break;
        }
    }

    vec3 col = vec3(sin(brightness * 6.2831), brightness, 1.0 - brightness);
    return vec4(col, brightness);  // Alpha based on brightness
}

// --------------------------------------------------------
// Main
// --------------------------------------------------------
void mxCacheShaderMain() {
    // Start with the base texture
    color = texture(samp, tc);

    // Normalized coordinates (center screen is [0,0])
    vec2 uv = 2.0 * tc - 1.0;

    // Define the cycle duration for changing position/seed
    float cycleDuration = 5.0;
    float cycleTime = mod(time_f, cycleDuration);
    float cycleIndex = floor(time_f / cycleDuration);

    // Random position for the Julia fractal (in screen space [-0.8, 0.8])
    vec2 randomPos = vec2(rand(vec2(cycleIndex, 1.0)), rand(vec2(cycleIndex, 2.0))) * 1.6 - 0.8;

    // Animated seed for the Julia fractal
    vec2 juliaSeed = vec2(sin(time_f * 0.3), cos(time_f * 0.3)) * 0.5;

    // Alpha fade-in/out effect
    float alpha = smoothstep(0.0, 1.0, cycleTime) * smoothstep(cycleDuration, cycleDuration - 1.0, cycleTime);

    // Draw the fractal
    vec4 juliaColor = drawRandomJulia(uv, randomPos, juliaSeed);
    juliaColor.a *= alpha;  // Apply alpha fade

    // Blend with the base texture
    color = mix(color, juliaColor, juliaColor.a);
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
