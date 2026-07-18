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
    // Common parameters
    vec2 center = vec2(0.5);
    vec3 purpleTint = vec3(0.7, 0.0, 0.7); // Strong purple color

    // Wave 1: Diagonal Red Wave
    float ripple1 = sin(tc.x * 12.0 + time_f * 5.0) * 0.03;
    ripple1 += sin(tc.y * 9.6 + time_f * 6.0) * 0.03;
    vec2 tc1 = tc + vec2(ripple1);
    tc1.y += sin(time_f * 2.5) * 0.02; // Vertical movement

    // Spiral effect
    vec2 pos1 = tc1 - center;
    float angle1 = length(pos1) * 8.0 + time_f * 3.0;
    mat2 rot1 = mat2(cos(angle1), -sin(angle1), sin(angle1), cos(angle1));
    tc1 = rot1 * pos1 + center;


    // Wave 2: Horizontal Blue Wave
    float ripple2 = sin(tc.x * 15.0 + time_f * 6.5) * 0.025;
    ripple2 += sin(tc.y * 4.5 + time_f * 4.5) * 0.025;
    vec2 tc2 = tc + vec2(ripple2 * 1.5, -ripple2 * 0.7);
    tc2.y += sin(time_f * 3.0) * 0.015; // Vertical movement

    // Reverse spiral
    vec2 pos2 = tc2 - center;
    float angle2 = -length(pos2) * 6.0 + time_f * 2.5;
    mat2 rot2 = mat2(cos(angle2), -sin(angle2), sin(angle2), cos(angle2));
    tc2 = rot2 * pos2 + center;

    // Wave 3: Vertical Combined Wave
    float ripple3 = sin(tc.x * 6.0 + time_f * 4.0) * 0.035;
    ripple3 += sin(tc.y * 14.0 + time_f * 5.2) * 0.035;
    vec2 tc3 = tc + vec2(ripple3 * 0.4, ripple3);
    tc3.y += sin(time_f * 4.5) * 0.025; // Vertical movement

    // Swirling spiral
    vec2 pos3 = tc3 - center;
    float angle3 = length(pos3) * 10.0 + time_f * 4.0;
    mat2 rot3 = mat2(cos(angle3), -sin(angle3), sin(angle3), cos(angle3));
    tc3 = rot3 * pos3 + center;

    // Sample texture with psychedelic combination
    vec3 c = vec3(0.0);
    c.r += texture(samp, tc1).r * 1.2;
    c.b += texture(samp, tc2).b * 1.2;
    c.rb += texture(samp, tc3).rb * 0.8;

    // Apply purple tint and boost intensity
    color = vec4(c * purpleTint * 1.5, 1.0);
    color = mix(color, texture(samp, tc), 0.5);
    color = vec4(color.rgb, texture(samp, tc).a);
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
