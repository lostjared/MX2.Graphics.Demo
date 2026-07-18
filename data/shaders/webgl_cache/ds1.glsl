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
uniform float time_f;
uniform sampler2D samp;
uniform vec2 iResolution;

void mxCacheShaderMain() {
    // Center coordinates and scale
    vec2 uv = tc * 2.0 - 1.0;

    // Polar coordinates conversion
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);

    // Kaleidoscope parameters
    const int slices = 6;  // Number of mirror slices
    float angleSlice = 2.0 * 3.14159 / float(slices);

    // Create rotational symmetry and mirroring
    angle = mod(angle, angleSlice * 2.0);
    angle = abs(angle - angleSlice);

    // Add time-based rotation
    angle += time_f * 0.5;

    // Dynamic distortion effects
    radius *= 1.0 + 0.1 * sin(time_f * 2.0 + angle * 5.0);
    angle += sin(time_f * 1.5 + radius * 5.0) * 0.3;

    // Convert back to Cartesian coordinates
    vec2 distortedUV = vec2(cos(angle), sin(angle)) * radius;

    // Create swirling effect
    distortedUV *= 0.5 + 0.3 * sin(time_f + radius * 3.0);

    // Mirror and tile pattern
    distortedUV = abs(fract(distortedUV * 1.5) * 2.0 - 1.0);

    // Final texture sampling with perspective warp
    vec2 finalUV = (distortedUV + 1.0) * 0.5;
    finalUV = fract(finalUV + vec2(time_f * 0.1, 0.0));

    color = texture(samp, finalUV);

    // Add color cycling effect (optional)
    color.rgb = mix(color.rgb, fract(color.rgb + time_f * 0.1), 0.2);
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
