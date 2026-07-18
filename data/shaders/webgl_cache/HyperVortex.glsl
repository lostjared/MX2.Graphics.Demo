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
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;

// --- UTILITIES ---
const float PI = 3.14159265359;

mat3 rotX(float a){float s=sin(a),c=cos(a);return mat3(1,0,0, 0,c,-s, 0,s,c);}
mat3 rotY(float a){float s=sin(a),c=cos(a);return mat3(c,0,s, 0,1,0, -s,0,c);}
mat3 rotZ(float a){float s=sin(a),c=cos(a);return mat3(c,-s,0, s,c,0, 0,0,1);}

// --- NOISE (From Previous Concept) ---
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
    for (int i = 0; i < 3; i++) { // Reduced iterations for performance
        v += a * noise(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}

// --- COLOR PALETTE ---
vec3 electricPalette(float t) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.00, 0.33, 0.67); // Blue/Cyan heavy
    return a + b * cos(6.28318 * (c * t + d));
}

void mxCacheShaderMain() {
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 center = vec2(0.5);

    // Normalize coordinates centered at 0,0
    vec2 p = (tc - center) * ar;

    // --- 1. THE TWIST (New Input) ---
    // We modify the twist to be dynamic and "deep"
    float len = length(p);
    float twistStrength = 3.0 * sin(time_f * 0.3);
    // Twist increases closer to center, creating a vortex feel
    float angle = atan(p.y, p.x) + twistStrength / (len + 0.1) + time_f * 0.5;

    // Convert back to cartesian, but keep the spiral nature
    vec2 twistedP = vec2(cos(angle), sin(angle)) * len;

    // --- 2. THE RIPPLE + NOISE (Combined Inputs) ---
    float rippleWavelength = 8.0;
    float rippleSpeed = 4.0;

    // Add FBM noise to the ripple calculation so it's not a perfect sine wave
    float noiseVal = fbm(twistedP * 5.0 + time_f);

    float rippleX = sin(twistedP.x * rippleWavelength + time_f * rippleSpeed + noiseVal * 2.0);
    float rippleY = cos(twistedP.y * rippleWavelength + time_f * rippleSpeed - noiseVal * 2.0);

    float rippleAmp = 0.05 + 0.05 * noiseVal; // Amplitude reacts to noise
    vec2 rippledP = twistedP + vec2(rippleX, rippleY) * rippleAmp;

    // --- 3. 3D PROJECTION (Previous Input) ---
    // We map the twisted/rippled 2D plane onto a 3D tumbling surface
    vec3 p3 = vec3(rippledP, 1.0);

    // Slow rotation
    mat3 R = rotX(time_f * 0.2) * rotY(time_f * 0.15) * rotZ(time_f * 0.1);
    vec3 r = R * p3;

    // Perspective division
    float zScale = 1.0 / (1.0 + r.z * 0.5);
    vec2 projUV = r.xy * zScale;

    // --- 4. MIRROR TILING (Previous Input) ---
    // Apply the "Fold" logic to the projected coordinates
    vec2 tiledUV = 1.0 - abs(1.0 - 2.0 * (projUV + 0.5));
    tiledUV = tiledUV - floor(tiledUV);

    // --- 5. CHROMATIC ABERRATION via TWIST INTENSITY ---
    // The more twisted the area (closer to center), the more the colors split
    float aberrationStr = 0.01 + 0.04 * (1.0 / (len + 0.5));
    vec2 offset = vec2(aberrationStr, 0.0);

    // Sample channels
    vec4 texR = texture(samp, clamp(tiledUV - offset, 0.0, 1.0));
    vec4 texG = texture(samp, clamp(tiledUV, 0.0, 1.0));
    vec4 texB = texture(samp, clamp(tiledUV + offset, 0.0, 1.0));

    // Combine texture
    vec3 finalColor = vec3(texR.r, texG.g, texB.b);

    // --- 6. ADDITIVE GLOW ---
    // Use the ripple intensity to add "electric" glowing highlights
    float glowMask = smoothstep(0.6, 0.95, abs(rippleX * rippleY));
    vec3 palette = electricPalette(len - time_f);

    // Mix texture with the palette based on the glow mask
    finalColor = mix(finalColor, finalColor + palette, glowMask * 0.8);

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
