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

void mxCacheShaderMain() {
    // 1. Center and fix aspect ratio
    vec2 uv = (tc - 0.5) * 2.0;
    uv.x *= iResolution.x / iResolution.y;

    // 2. Deep Fractal Setup
    // 'p' is our moving point, 'c' is the constant based on mouse or time
    vec2 p = uv;
    float iters = 0.0;
    const float max_iters = 64.0;

    // Smoothly zoom in and out over time
    float zoom = pow(0.5, mod(time_f * 0.5, 10.0));
    p *= zoom;

    // 3. The Escape-Time Loop (The "Deep" part)
    // We iterate the function: z = z^2 + c
    for(float i = 0.0; i < max_iters; i++) {
        // Space folding (Abs creates symmetry, like your old x%2 check but infinite)
        p = abs(p) / dot(p, p) - vec2(0.8, 0.5 + 0.1 * sin(time_f * 0.3));

        if (length(p) > 20.0) break;
        iters++;
    }

    // 4. Color Logic (Integrating your original style)
    // Use the iteration count to pick a color depth
    float normIters = iters / max_iters;
    vec4 texColor = texture(samp, tc + p * 0.02); // Distorted texture lookup

    vec3 fractalColor;
    fractalColor.r = normIters * 2.0;
    fractalColor.g = sin(iters * 0.5 + time_f);
    fractalColor.b = length(p) * 0.1;

    // Apply your swapping/inversion logic
    float temp = fractalColor.r;
    fractalColor.r = fractalColor.b;
    fractalColor.b = temp;

    // Final output with your signature sin-time oscillation
    vec3 finalColor = (sin(time_f) > 0.0) ? vec3(1.0) - fractalColor : fractalColor;

    // Mix with original texture for a "ghostly" fractal overlay
    vec3 composite = mix(texColor.rgb, finalColor, 0.7);

    color = vec4(sin(composite * time_f), 1.0);
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
