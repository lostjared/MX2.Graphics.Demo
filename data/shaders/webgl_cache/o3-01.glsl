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
uniform vec4 iMouse;

// Convert an HSV color to RGB.
vec3 hsv2rgb(vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0),
                             6.0) - 3.0) - 1.0,
                     0.0, 1.0);
    return c.z * mix(vec3(1.0), rgb, c.y);
}

void mxCacheShaderMain() {
    // Start with the texture coordinate.
    vec2 uv = tc;

    // Center the coordinate system around (0,0)
    uv = uv * 2.0 - 1.0;

    // Convert to polar coordinates.
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);

    // Compute a swirl offset that changes with both the radius and time.
    // Adjust the multiplier (10.0) and time factor (3.0) to tune the effect.
    float swirl = sin(radius * 10.0 - time_f * 3.0) * 0.5;

    // Optionally, modulate swirl strength with the mouse's x position.
    float mouseFactor = (iMouse.x > 0.0) ? (iMouse.x / iResolution.x) : 1.0;
    swirl *= mouseFactor;

    // Add the swirl offset to the polar angle.
    angle += swirl;

    // Convert back to Cartesian coordinates.
    uv = vec2(cos(angle), sin(angle)) * radius;

    // Transform back to standard texture coordinate space [0,1].
    uv = (uv + 1.0) * 0.5;

    // Sample the texture using the distorted coordinates.
    vec4 texColor = texture(samp, uv);

    // Create a dynamic hue based on the radius and time.
    // This produces a hue that continuously cycles for a vivid color effect.
    float hue = fract(time_f * 0.1 + radius);
    vec3 hsv = vec3(hue, 1.0, 1.0);
    vec3 psychedelicColor = hsv2rgb(hsv);

    // Mix the original texture color with the psychedelic color.
    // The mix factor (0.7) can be adjusted for more or less color dominance.
    vec3 finalColor = mix(texColor.rgb, psychedelicColor, 0.7);

    color = vec4(finalColor, texColor.a);
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
