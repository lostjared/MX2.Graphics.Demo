#version 300 es
precision highp float;
precision highp int;
uniform float iSpeed;
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


uniform sampler2D samp;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;

out vec4 color;
in vec2 TexCoord;
#define tc mxCacheTexCoord

// Controls
uniform float iAmplitude;
uniform float iFrequency;
uniform float iBrightness;
uniform float iContrast; // Higher contrast for neon look
uniform float iSaturation; // Boost saturation for deep colors
uniform float iHueShift;
uniform float iZoom;
uniform float iRotation;

// --- COLOR HELPER FUNCTIONS ---

vec3 adjustBrightness(vec3 col, float b) { return col * b; }
vec3 adjustContrast(vec3 col, float c) { return (col - 0.5) * c + 0.5; }
vec3 adjustSaturation(vec3 col, float s) {
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    return mix(vec3(gray), col, s);
}

// --- NEON PALETTE GENERATOR ---
// Strictly cycles Pink -> Purple -> Blue
vec3 getNeonPalette(float t) {
    // Phase shift t to loop
    float x = fract(t + iHueShift);

    // Define Neon Anchors
    vec3 colBlue   = vec3(0.05, 0.1, 1.0);  // Deep Electric Blue
    vec3 colPurple = vec3(0.6, 0.0, 0.9);   // Neon Purple
    vec3 colPink   = vec3(1.0, 0.05, 0.6);  // Hot Pink

    // Smooth interpolation between the 3 colors
    if (x < 0.33) {
        return mix(colBlue, colPurple, x * 3.0);
    } else if (x < 0.66) {
        return mix(colPurple, colPink, (x - 0.33) * 3.0);
    } else {
        return mix(colPink, colBlue, (x - 0.66) * 3.0);
    }
}

// --- COORDINATE HELPERS ---

vec2 wrapUV(vec2 tc) {
    return 1.0 - abs(1.0 - 2.0 * fract(tc * 0.5));
}

vec4 mxTexture(sampler2D tex, vec2 tc) {
    vec2 ts = vec2(textureSize(tex, 0));
    vec2 eps = 0.5 / ts;
    vec2 uv = wrapUV(tc);
    vec2 sampleUV = clamp(uv, eps, 1.0 - eps);
    return texture(tex, sampleUV);
}

vec2 rotate2D(vec2 p, float a) {
    float c = cos(a); float s = sin(a);
    return mat2(c, -s, s, c) * p;
}

// --- WAVE FUNCTION ---
// Adds the rhythmic undulating waves
vec2 sineWave(vec2 p, float t) {
    float waveAmp = 0.1 * iAmplitude;
    float waveFreq = 4.0 * iFrequency;

    // Primary vertical wave
    p.y += sin(p.x * waveFreq + t) * waveAmp;

    // Secondary horizontal drift
    p.x += cos(p.y * waveFreq * 0.5 + t) * (waveAmp * 0.5);

    return p;
}

// --- NOISE ---

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash(i + vec2(0.0, 0.0));
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.50));
    for (int i = 0; i < 5; i++) {
        v += a * noise(p);
        p = rot * p * 2.0 + vec2(2.0);
        a *= 0.5;
    }
    return v;
}

// --- LIQUID NEON LOGIC ---

vec3 sampleLiquid(vec2 uv, float t, float strength, vec2 center, vec2 res) {
    float aspect = res.x / res.y;
    vec2 p = (uv - center) * vec2(aspect, 1.0);

    // 1. Initial Transform
    p = rotate2D(p, iRotation);
    float zoom = max(iZoom, 0.1);
    p /= zoom;

    // 2. APPLY SINE WAVES (The "Waves going through it")
    // We apply this BEFORE the noise so the liquid flows *over* the waves
    p = sineWave(p, t * 2.0);

    // 3. RECURSIVE DOMAIN WARPING (The Liquid Core)
    vec2 q = vec2(0.0);
    q.x = fbm(p + vec2(0.0, 0.0) + 0.05*t);
    q.y = fbm(p + vec2(5.2, 1.3) + 0.05*t);

    vec2 r = vec2(0.0);
    r.x = fbm(p + 4.0*q + vec2(t * 0.15));
    r.y = fbm(p + 4.0*q + vec2(t * 0.05, 2.8));

    float f = fbm(p + 4.0*r);

    // 4. TEXTURE SAMPLING
    // Use the warped 'r' vector to pull texture colors
    vec2 fluidUV = uv + (r * 0.15 * strength * iAmplitude);

    // Add the sine wave to the texture lookup too for coherence
    fluidUV = sineWave(fluidUV, t * 2.0);

    vec3 texCol = mxTexture(samp, fluidUV).rgb;

    // 5. COLORING (Neon Palette)
    // We drive the palette with the noise magnitude 'length(q)' and time
    vec3 neon = getNeonPalette(length(q) + f + t * 0.1);

    // 6. COMPOSITION
    // Darken the texture slightly so the neon pops
    vec3 base = texCol * 0.5;

    // Add the neon color where the liquid turbulence (f) is high
    vec3 col = mix(base, neon, f * iSaturation);

    // Add "Electric" ridges
    // Creates bright lines at the edges of the waves
    float ridges = 1.0 - abs(f * 2.0 - 1.0);
    ridges = pow(ridges, 3.0); // Sharpen
    col += neon * ridges * strength * 0.8;

    // Final tint to ensure everything stays in the Pink/Purple/Blue realm
    // This prevents the underlying texture colors from breaking the theme too much
    col = mix(col, col * vec3(0.8, 0.5, 1.0), 0.3);

    return col;
}

void mxCacheShaderMain() {
    vec2 uv = tc;
    uv = wrapUV(uv);

    float t = (time_f * 3.0) * (0.2 + iFrequency * 0.1);
    float ampControl = clamp(iAmplitude, 0.0, 2.0);
    float strength = 1.0 + (ampControl * 0.5);

    vec2 center = vec2(0.5);
    if (iMouse.z > 0.0) {
        center = iMouse.xy / iResolution;
    }

    // Chromatic Aberration (Shifted for Neon effect)
    // Red and Blue channels are offset to create a "glitchy" neon edge
    vec2 offset = vec2(0.005 * strength, 0.0);

    vec3 col;
    col.r = sampleLiquid(uv + offset, t, strength, center, iResolution).r;
    col.g = sampleLiquid(uv,          t, strength, center, iResolution).g; // Green is often kept low in neon
    col.b = sampleLiquid(uv - offset, t, strength, center, iResolution).b;

    // Post Processing
    col = adjustBrightness(col, iBrightness);
    col = adjustContrast(col, iContrast);
    col = adjustSaturation(col, iSaturation);

    // Blue-ish Vignette
    vec2 vUV = uv * (1.0 - uv.yx);
    float vig = vUV.x * vUV.y * 15.0;
    vig = pow(vig, 0.15);
    col *= vig;
    // Tint vignette dark purple instead of black
    col = mix(vec3(0.1, 0.0, 0.2), col, vig);

    color = vec4(col, 1.0);
}
void main() {
    mxCacheTexCoord = mxCacheApplyCoordinateAdjustments(
        TexCoord, 1.0, 1.0, 0.0, iQuality, vec2(textureSize(samp, 0)));
    vec4 mxCacheInputColor = texture(samp, mxCacheTexCoord);
    mxCacheShaderMain();
    vec4 mxCacheEffectColor = color;
    mxCacheEffectColor.rgb = mxCacheApplyColorAdjustments(
        mxCacheEffectColor.rgb, 1.0, 1.0, 1.0, 0.0);
    if (iDebugMode > 0.5 && TexCoord.x < 0.5) {
        mxCacheEffectColor = mxCacheInputColor;
    }
    color = mxCacheEffectColor;
}
