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
uniform float iContrast;
uniform float iSaturation;
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

// IQ Cosine Palette
vec3 palette(float t) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263, 0.416, 0.557);
    return a + b * cos(6.28318 * (c * t + d + iHueShift));
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
    return textureLod(tex, sampleUV, 0.0);
}

vec2 rotate2D(vec2 p, float a) {
    float c = cos(a); float s = sin(a);
    return mat2(c, -s, s, c) * p;
}

// --- NOISE ---

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; i++) {
        v += a * noise(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}

// --- RIPPLE BEND EXPAND LOGIC ---

// Calculates the distortion and returns the modified point + the intensity of distortion
vec3 rippleBendExpand(vec2 p, float t, float strength) {
    float d = length(p);

    // 1. EXPAND (Breathing/Pulse)
    float expansion = 1.0 + (sin(t * 2.0) * 0.1 * strength);
    p /= expansion;

    // 2. RIPPLE (Sine wave propagation)
    // Frequency increases with distance
    float wavePhase = d * (10.0 * iFrequency) - (t * 5.0);
    float ripple = sin(wavePhase);

    // Displace outward based on ripple
    p += (p / (d+0.001)) * ripple * 0.05 * strength;

    // 3. BEND (Angular Twist)
    float angle = atan(p.y, p.x);
    // Twist angle based on distance and ripple phase
    float twist = sin(d * 4.0 - t) * 0.5 * strength;
    angle += twist;

    // Reconstruct P
    p = vec2(cos(angle), sin(angle)) * length(p);

    // Return modified point (xy) and the amount of distortion (z) for "Extraction"
    return vec3(p, abs(ripple) + abs(twist));
}

vec2 kaleido(vec2 p, float slices) {
    float pi = 3.14159265359;
    float r = length(p);
    float a = atan(p.y, p.x);
    float sector = pi * 2.0 / slices;
    a = mod(a, sector);
    a = abs(a - sector * 0.5);
    return vec2(cos(a), sin(a)) * r;
}

// --- HYBRID RENDER LOGIC ---

vec3 sampleHybrid(vec2 uv, float t, float strength, vec2 center, vec2 res) {
    float aspect = res.x / res.y;
    vec2 p = (uv - center) * vec2(aspect, 1.0);

    // Initial Zoom/Rot
    p = rotate2D(p, iRotation);
    float zoom = max(iZoom, 0.01);
    p /= zoom;

    // --- APPLY RIPPLE / BEND / EXPAND ---
    // We capture the distortion intensity in 'distort.z'
    vec3 distort = rippleBendExpand(p, t, strength);
    p = distort.xy;
    float extractMask = distort.z; // This is the "Force" of the bend at this pixel

    // Domain Warping (Noise)
    vec2 q = vec2(fbm(p + t * 0.1), fbm(p + vec2(5.2, 1.3)));
    vec2 r = vec2(fbm(p + 4.0 * q + t * 0.2), fbm(p + q));
    p += r * (0.15 * strength);

    // Fractal Folding (Transform)
    float slices = 8.0; // Fixed slices for stability
    p = kaleido(p, slices);

    int iterations = 4;
    float scale = 1.3;
    float shift = 0.2 * strength;

    for(int i = 0; i < iterations; i++) {
        p = abs(p);
        p -= shift;
        p *= scale;
        p = rotate2D(p, t * 0.1 + float(i));
    }

    // Map back to UV space for texture
    vec2 finalUV = p * 0.5 + center;

    // Sample Texture
    vec3 texCol = mxTexture(samp, finalUV).rgb;

    // --- EXTRACT LOGIC ---
    // We use the 'extractMask' (calculated from the bend/ripple earlier)
    // to separate the image into "Energy" and "Matter".

    // 1. Define the "Energy" color based on palette
    vec3 energyCol = palette(length(p) * 0.2 + t * 0.5 + extractMask);

    // 2. Mix based on how hard the geometry was bent
    // Areas with high distortion get the energy color
    vec3 finalCol = mix(texCol, energyCol, smoothstep(0.2, 1.5, extractMask));

    // 3. Additive Glow for the "Extract" effect
    finalCol += energyCol * extractMask * 0.5 * strength;

    return finalCol;
}

void mxCacheShaderMain() {
    vec2 uv = tc;

    float t = time_f * (0.2 + iFrequency * 0.2);
    float ampControl = clamp(iAmplitude, 0.0, 3.0);
    float strength = 0.8 + (ampControl * 0.5);

    vec2 center = vec2(0.5);
    if (iMouse.z > 0.0) {
        center = iMouse.xy / iResolution;
    }

    // Chromatic Aberration (RGB Split)
    // We offset the start position for each channel based on the ripple strength
    vec2 offset = vec2(0.005 * strength, 0.0);

    vec3 col;
    col.r = sampleHybrid(uv + offset, t, strength, center, iResolution).r;
    col.g = sampleHybrid(uv,          t, strength, center, iResolution).g;
    col.b = sampleHybrid(uv - offset, t, strength, center, iResolution).b;

    // Post Processing
    col = adjustContrast(col, iContrast);
    col = adjustSaturation(col, iSaturation);
    col = adjustBrightness(col, iBrightness);

    // Darken edges slightly
    float vig = 1.0 - length(uv - 0.5);
    col *= smoothstep(0.0, 1.2, vig);

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
