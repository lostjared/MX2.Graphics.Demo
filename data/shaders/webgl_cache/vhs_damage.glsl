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

// Random function for noise effects
float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

// Chromatic aberration effect
vec3 chromaticAberration(vec2 uv, float intensity) {
    float r = texture(samp, uv + vec2(intensity, 0.0)).r;
    float g = texture(samp, uv).g;
    float b = texture(samp, uv - vec2(intensity, 0.0)).b;
    return vec3(r, g, b);
}

void mxCacheShaderMain() {
    vec2 uv = tc;

    // Tape stretch/wobble
    uv.y += sin(time_f * 2.0 + uv.x * 20.0) * 0.005;

    // Vertical jitter
    float vjitter = (rand(vec2(time_f)) - 0.5) * 0.02;
    uv.y = mod(uv.y + vjitter, 1.0);

    // Horizontal jitter
    uv.x += sin(time_f * 10.0) * 0.005;

    // Head switching effect (horizontal glitch line)
    float scanLine = fract(time_f * 0.5);
    if(abs(uv.y - scanLine) < 0.005 + rand(vec2(time_f)) * 0.01) {
        uv.x += (rand(vec2(uv.y + time_f)) - 0.5) * 0.1;
    }

    // Chromatic aberration
    vec3 col = chromaticAberration(uv, 0.003 + rand(uv) * 0.003);

    // Add noise
    float noiseIntensity = 0.3;
    vec3 noise = vec3(rand(uv * time_f)) * noiseIntensity;
    col += noise;

    // Horizontal scan lines
    float scanLines = sin(uv.y * 800.0 + time_f * 10.0) * 0.1;
    col -= scanLines * 0.1;

    // Color bleed (red and blue channels lag)
    vec2 bleedOffset = vec2(0.002 + sin(time_f) * 0.001, 0.0);
    col.r = texture(samp, uv + bleedOffset).r;
    col.b = texture(samp, uv - bleedOffset).b;

    // Tape damage (random dropouts)
    if(rand(vec2(time_f * 0.1, uv.y)) > 0.99) {
        col *= 0.1 + rand(uv) * 0.5;
    }

    // VHS color distortion
    col = mix(col, col.grb, sin(time_f) * 0.1);

    // Flicker effect
    col *= 0.9 + 0.1 * rand(vec2(time_f * 0.5));

    // Add VHS tracking lines
    float trackLines = step(0.995, rand(vec2(uv.y * 100.0, time_f)));
    col += trackLines * 0.3;

    // Output final color
    color = vec4(col, 1.0);

    // Tape edge distortion
    color *= smoothstep(0.0, 0.1, uv.y) * smoothstep(1.0, 0.9, uv.y);
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
