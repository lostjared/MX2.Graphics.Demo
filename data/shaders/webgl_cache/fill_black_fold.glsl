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

// Neon Palette Generator from your second shader
vec3 neonGradient(float t) {
    return 0.5 + 0.5 * cos(6.28318 * (t + vec3(0.0, 0.33, 0.67)));
}

void mxCacheShaderMain() {
    vec3 texCol = texture(samp, tc).rgb;

    // Threshold check for "black" pixels
    if(texCol.r < 0.3 && texCol.g < 0.3 && texCol.b < 0.3) {
        // 1. Setup Coordinates
        vec2 uv = (tc * 2.0 - 1.0);
        float aspect = iResolution.x / iResolution.y;
        uv.x *= aspect;

        float d = length(uv);
        float lensStrength = 1.5;

        // 2. Lighting / Normal math
        vec3 normal = normalize(vec3(uv, 1.0 / lensStrength));
        float fisheyeRadius = atan(d, 1.0);
        vec2 distortedUV = normalize(uv + 1e-6) * fisheyeRadius;

        float t = time_f * 0.8;
        float r_dist = length(distortedUV);
        float angle = atan(distortedUV.y, distortedUV.x);

        // 3. Spiral Calculation
        // The formula for the spiral angle is:
        // $spiral = \theta + 3.0 \cdot \ln(r + 0.1) - 1.5t$
        float spiral = angle + (log(r_dist + 0.1) * 3.0) - t * 1.5;

        float r = sin(spiral * 3.0 + t);
        float g = sin(spiral * 3.0 + t + 2.094);
        float b = sin(spiral * 3.0 + t + 4.188);

        vec3 spiralCol = vec3(r, g, b) * 0.5 + 0.5;

        // 4. Shading
        vec3 lightDir = normalize(vec3(sin(time_f), cos(time_f), 1.0));
        float diff = max(dot(normal, lightDir), 0.0);
        float spec = pow(max(dot(reflect(-lightDir, normal), vec3(0,0,1)), 0.0), 16.0);

        vec3 finalSpiral = spiralCol * (diff + 0.5) + spec;

        // Apply the neon gradient and vignette
        finalSpiral *= neonGradient(time_f);
        finalSpiral *= smoothstep(2.0, 0.5, d);

        color = vec4(finalSpiral, alpha);
    }
    else {
        // Leave the other colors as they are
        color = vec4(texCol, 1.0);
    }
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
