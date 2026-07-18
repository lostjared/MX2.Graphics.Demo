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

// --- NEON PALETTE ---
// Cycles strictly Pink -> Purple -> Blue
vec3 getNeonPalette(float t) {
    float x = fract(t + iHueShift);

    vec3 colBlue   = vec3(0.05, 0.2, 1.0);   // Electric Blue
    vec3 colPurple = vec3(0.7, 0.0, 1.0);   // Deep Neon Purple
    vec3 colPink   = vec3(1.0, 0.05, 0.5);  // Hot Pink

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
vec2 sineWave(vec2 p, float t) {
    float waveAmp = 0.05 * iAmplitude;
    float waveFreq = 3.0 * iFrequency;
    p.y += sin(p.x * waveFreq + t) * waveAmp;
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
    return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
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

// --- UNIFIED FLUID LOGIC ---

vec3 sampleMergedLiquid(vec2 uv, float t, float strength, vec2 center, vec2 res) {
    float aspect = res.x / res.y;
    vec2 p = (uv - center) * vec2(aspect, 1.0);

    // 1. Initial Transform
    p = rotate2D(p, iRotation);
    float zoom = max(iZoom, 0.1);
    p /= zoom;

    // 2. Wave Distortion
    p = sineWave(p, t * 1.5);

    // 3. Fluid Simulation (Warping the coordinates)
    vec2 q = vec2(0.0);
    q.x = fbm(p + vec2(0.0, 0.0) + 0.05*t);
    q.y = fbm(p + vec2(5.2, 1.3) + 0.05*t);

    vec2 r = vec2(0.0);
    r.x = fbm(p + 4.0*q + vec2(t * 0.2));
    r.y = fbm(p + 4.0*q + vec2(t * 0.1, 2.8));

    // Calculate final distortion amount
    // 'r' contains the fluid motion vectors
    vec2 distortion = r * 0.3 * strength * iAmplitude;

    // 4. MAP TEXTURE TO THE FLUID
    // We apply the fluid distortion (r) directly to the UVs used to lookup the texture
    vec2 fluidUV = uv + distortion;

    // Also apply the sine wave to the lookup so the image undulates
    fluidUV = sineWave(fluidUV, t * 1.5);

    vec3 texCol = mxTexture(samp, fluidUV).rgb;

    // 5. COLOR MAPPING
    // Generate the neon palette using the SAME warping vectors (q and r)
    // This ensures color bands align perfectly with texture distortion
    float pattern = length(q) + length(r) + t * 0.1;
    vec3 neon = getNeonPalette(pattern);

    // 6. BLENDING (The "Texture Mapped Rainbow")

    // Calculate the brightness of the underlying texture pixel
    float lum = dot(texCol, vec3(0.299, 0.587, 0.114));

    // Multiply texture by neon.
    // This tints the texture while preserving its details.
    // We multiply by 2.0 to ensure the colors are vibrant, not dark.
    vec3 finalCol = texCol * neon * 2.5;

    // Add a little bit of the original texture back in for definition
    // (Optional: remove this line for purely liquid look)
    finalCol = mix(finalCol, texCol * vec3(0.5, 0.3, 0.8), 0.2);

    return finalCol;
}

void mxCacheShaderMain() {
    vec2 uv = tc;
    uv = wrapUV(uv);

    float t = time_f * (0.2 + iFrequency * 0.1);
    float ampControl = clamp(iAmplitude, 0.0, 2.0);
    float strength = 1.0 + (ampControl * 0.5);

    vec2 center = vec2(0.5);
    if (iMouse.z > 0.0) {
        center = iMouse.xy / iResolution;
    }

    // Chromatic Aberration
    // Separation is driven by the fluid strength
    vec2 offset = vec2(0.004 * strength, 0.0);

    vec3 col;
    // We sample the entire merged liquid function per channel
    col.r = sampleMergedLiquid(uv + offset, t, strength, center, iResolution).r;
    col.g = sampleMergedLiquid(uv,          t, strength, center, iResolution).g;
    col.b = sampleMergedLiquid(uv - offset, t, strength, center, iResolution).b;

    // Post Processing
    col = adjustBrightness(col, iBrightness);
    col = adjustContrast(col, iContrast);
    col = adjustSaturation(col, iSaturation);

    // Deep Purple Vignette
    vec2 vUV = uv * (1.0 - uv.yx);
    float vig = vUV.x * vUV.y * 15.0;
    vig = pow(vig, 0.2);
    col *= vig;
    col = mix(vec3(0.05, 0.0, 0.1), col, vig); // Fade to deep purple edges

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
