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
uniform float alpha;

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void mxCacheShaderMain() {
    // 1. Correct Aspect Ratio & Centering
    vec2 uv = (tc * 2.0 - 1.0);
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;

    // 2. Full-Screen Spherical/Fisheye Logic
    float d = length(uv);

    // Instead of a fixed sphereRadius, we use a 'lens strength'
    // This creates a bulge that covers the whole screen.
    float lensStrength = 1.5;
    float z = sqrt(lensStrength * lensStrength + d * d); // Hyperbolic-style depth

    // Create normals for shading (lighting) across the whole screen
    vec3 normal = normalize(vec3(uv, 1.0 / lensStrength));

    // Distort UVs for the spiral - no more "if" discard block
    float fisheyeRadius = atan(d, 1.0);
    vec2 distortedUV = normalize(uv + 1e-6) * fisheyeRadius;

    // 3. Spiral Logic (Ping-Ponging the direction and tightness)
    float t = time_f * 0.8;
    float pTime = pingPong(time_f * 0.5, 2.0); // Used to oscillate spiral intensity

    float r_dist = length(distortedUV);
    float angle = atan(distortedUV.y, distortedUV.x);

    // Enhanced Spiral: we use pTime to make it 'unwind' and 'rewind'
    float spiral = angle + (log(r_dist + 0.1) * (2.0 + pTime)) - t * 1.5;

    // Color generation (Neon Spectrum)
    float r = sin(spiral * 3.0 + t);
    float g = sin(spiral * 3.0 + t + 2.094);
    float b = sin(spiral * 3.0 + t + 4.188);

    vec3 spiralCol = vec3(r, g, b) * 0.5 + 0.5;

    // 4. Shading & Lighting (Applied globally)
    vec3 lightDir = normalize(vec3(sin(time_f), cos(time_f), 1.0)); // Moving light
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(reflect(-lightDir, normal), vec3(0,0,1)), 0.0), 16.0);

    // 5. Final Mix
    vec3 texColor = texture(samp, tc).rgb;

    // We mix the spiral with the texture based on the distance from center
    // to keep the center clear or create an 'aura' feel
    float mixFactor = smoothstep(0.0, 1.5, d);

    vec3 finalCol = mix(texColor, spiralCol * (diff + 0.5) + spec, 0.7);

    // Subtle vignette to focus the screen
    finalCol *= smoothstep(2.0, 0.5, d);

    color = vec4(finalCol, alpha);
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
