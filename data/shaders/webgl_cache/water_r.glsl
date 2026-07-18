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

void mxCacheShaderMain() {
    // Channel-specific parameters
    float speedR = 5.0;
    float amplitudeR = 0.03;
    float wavelengthR = 10.0;

    float speedG = 6.5;
    float amplitudeG = 0.025;
    float wavelengthG = 12.0;

    float speedB = 4.0;
    float amplitudeB = 0.035;
    float wavelengthB = 8.0;

    // Red channel wave (diagonal)
    float rippleR = sin(tc.x * wavelengthR + time_f * speedR) * amplitudeR;
    rippleR += sin(tc.y * wavelengthR * 0.8 + time_f * speedR * 1.2) * amplitudeR;
    vec2 rippleTC_R = tc + vec2(rippleR, rippleR);

    // Green channel wave (horizontal emphasis)
    float rippleG = sin(tc.x * wavelengthG * 1.5 + time_f * speedG) * amplitudeG;
    rippleG += sin(tc.y * wavelengthG * 0.3 + time_f * speedG * 0.7) * amplitudeG;
    vec2 rippleTC_G = tc + vec2(rippleG, -rippleG * 0.5);

    // Blue channel wave (vertical emphasis)
    float rippleB = sin(tc.x * wavelengthB * 0.5 + time_f * speedB) * amplitudeB;
    rippleB += sin(tc.y * wavelengthB * 1.7 + time_f * speedB * 1.3) * amplitudeB;
    vec2 rippleTC_B = tc + vec2(rippleB * 0.3, rippleB);

    // Sample texture with different displacements for each channel
    vec4 originalColor = texture(samp, tc);
    originalColor.r = texture(samp, rippleTC_R).r;
    originalColor.g = texture(samp, rippleTC_G).g;
    originalColor.b = texture(samp, rippleTC_B).b;

    color = originalColor;
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
