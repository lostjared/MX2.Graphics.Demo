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

uniform float time_f;          // Time value for animation
uniform sampler2D samp;         // Texture sampler
uniform vec2 iResolution;      // Screen resolution
uniform vec4 iMouse;           // Mouse position

// Parameters to control the glitch effect
const float frequency = 0.5; // Main frequency of the warp
const float strength = 1.0;   // Intensity of the warp

void mxCacheShaderMain() {
    vec2 warpedTexCoord = tc;

    // Add noise-based distortion
    float noise = fract(sin(warpedTexCoord.x * frequency + time_f) *
4.0);
    noise += fract(sin(warpedTexCoord.y * frequency * 2.0 + time_f) *
2.0);
    noise *= strength;

    // Combine with mouse position for interactive warping
    vec2 mousePos = iMouse.xy;
    mousePos.x = sin(mousePos.x * 16.0 + time_f);
    mousePos.y = sin(mousePos.y * 16.0 + time_f);

    warpedTexCoord += noise * mousePos * strength;

    // Apply multiple layers of distortion
    float layer1 = fract(sin(tc.x * 4.0 + time_f) * 2.0);
    float layer2 = fract(sin(tc.y * 4.0 * 2.0 + time_f) * 2.0);
    float layer3 = fract(sin((tc.x + tc.y) * 8.0 + time_f) * 1.0);

    // Combine all layers
    vec2 finalTexCoord = tc + (layer1 + layer2 * mousePos.x + layer3 *
mousePos.y) * strength;

    color = mix(texture(samp, sin(finalTexCoord * time_f)), texture(samp, tc), 0.5);
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
