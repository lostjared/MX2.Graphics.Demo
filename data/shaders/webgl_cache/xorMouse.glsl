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
uniform vec4 iMouse;

vec3 hueShift(vec3 color, float hue) {
    vec3 k = vec3(0.57735, 0.57735, 0.57735);
    float cosAngle = cos(hue);
    return color * cosAngle + cross(k, color) * sin(hue) + k * dot(k, color) * (1.0 - cosAngle);
}

void mxCacheShaderMain() {
    vec4 baseColor = texture(samp, tc);
    vec2 mouseNorm = iMouse.xy / iResolution.xy;
    vec2 clickNorm = iMouse.zw / iResolution.xy;

    // Calculate drag vector and strength
    vec2 dragVec = mouseNorm - clickNorm;
    float dragStrength = smoothstep(0.0, 0.5, length(dragVec));
    vec2 dragDir = normalize(dragVec + vec2(0.0001));

    // Calculate color shift parameters
    float hueAngle = atan(dragDir.y, dragDir.x);
    float shiftAmount = dragStrength * 2.0;

    // Animate return when released
    float returnSpeed = 2.0;
    float timeDecay = exp(-time_f * returnSpeed * (1.0 - step(0.5, iMouse.z)));
    shiftAmount *= mix(timeDecay, 1.0, step(0.5, iMouse.z));

    // Apply directional hue shift
    vec3 shiftedColor = hueShift(baseColor.rgb, hueAngle * shiftAmount);

    // Add chromatic aberration
    vec2 redOffset = dragDir * shiftAmount * 0.02;
    vec2 greenOffset = dragDir * shiftAmount * 0.01;
    vec3 finalColor = vec3(
        texture(samp, tc - redOffset).r,
        texture(samp, tc - greenOffset).g,
        texture(samp, tc).b
    );

    // Blend between original and shifted colors
    finalColor = mix(finalColor, shiftedColor, dragStrength);

    color = vec4(finalColor, baseColor.a);
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
