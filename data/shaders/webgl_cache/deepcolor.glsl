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

#define PI 3.14159265359
#define TAU 6.28318530718

mat2 rotate(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat2(c, -s, s, c);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 waveDistort(vec2 uv, float intensity) {
    vec2 d = vec2(0.0);
    d.x = intensity * sin(uv.y * 5.0 + time_f * 2.0) * cos(uv.x * 3.0 + time_f);
    d.y = intensity * cos(uv.x * 4.0 + time_f * 1.5) * sin(uv.y * 2.0 + time_f);
    return uv + d * 0.1;
}

vec3 chromaticAberration(sampler2D tex, vec2 uv, float amount) {
    float r = texture(tex, uv + vec2(amount, 0.0)).r;
    float g = texture(tex, uv).g;
    float b = texture(tex, uv - vec2(amount, 0.0)).b;
    return vec3(r, g, b);
}

void mxCacheShaderMain() {
    // Normalized pixel coordinates
    vec2 uv = tc;

    // Create dynamic wave distortion
    vec2 distortedUV = waveDistort(uv, 1.0 + sin(time_f * 0.5));

    // Add rotating distortion
    distortedUV -= vec2(0.5);
    distortedUV *= rotate(time_f * 0.3);
    distortedUV += vec2(0.5);

    // Get base texture color with chromatic aberration
    vec3 texColor = chromaticAberration(samp, distortedUV, 0.005 * (1.0 + sin(time_f)));

    // Create color phase shift
    float hueShift = 0.5 * sin(time_f * 0.3) + 0.5 * sin(uv.x * 10.0 + time_f);
    vec3 rainbow = hsv2rgb(vec3(fract(time_f * 0.1 + uv.x * 0.5 + uv.y * 0.3), 0.8, 0.6));

    // Create pulsing wave pattern
    float wave = 0.5 + 0.5 * sin(uv.x * 20.0 + time_f * 5.0) *
                        cos(uv.y * 15.0 + time_f * 4.0);

    // Combine texture with color effects
    vec3 finalColor = texColor * (1.0 + wave * 0.5) + rainbow * wave * 0.3;

    // Add scanline effect
    float scanline = 0.9 + 0.1 * sin(uv.y * iResolution.y * 1.5 + time_f * 10.0);
    finalColor *= scanline;

    // Add vignette
    vec2 vigUV = uv * (1.0 - uv) * 2.0;
    float vignette = pow(vigUV.x * vigUV.y * 15.0, 0.3);
    finalColor *= vignette;

    color = vec4(finalColor, 1.0);
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
