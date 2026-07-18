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

void mxCacheShaderMain() {
    // 1. Center the coordinates (-1.0 to 1.0) and fix aspect ratio
    vec2 uv = (tc - 0.5) * 2.0;
    uv.x *= iResolution.x / iResolution.y;

    // 2. Simple Circle Math (No Triangles)
    // 'dist' is the radius from the center
    // 'angle' is the rotation around the center
    float dist = length(uv);
    float angle = atan(uv.x, uv.y) / 3.14159;

    // 3. Create Seamless Warp Coordinates
    // Using log(dist) makes the "tunnel" perspective smooth and infinite
    // Adding time_f to 'dist' makes it zoom; adding it to 'angle' makes it spin
    vec2 warpedTC;
    warpedTC.x = angle + (time_f * 0.05);
    warpedTC.y = (1.0 / (dist + 0.01)) + (time_f * 0.5);

    // 4. The Mirror Trick (Removes all rough edges/seams)
    // 'fract' keeps it in 0-1 range, then the 'abs' math mirrors it
    // so the edges of the texture always meet their own reflection.
    vec2 finalTC = abs(fract(warpedTC * 0.5) * 2.0 - 1.0);

    // Sample the texture
    vec4 texColor = texture(samp, finalTC);

    // 5. Psychedelic Color Grade (Matching your reference)
    // Cycles colors based on distance and time
    vec3 rainbow = 0.5 + 0.5 * cos(6.28318 * (dist - time_f * 0.4 + vec3(0.0, 0.33, 0.67)));

    // Vignette: Darkens the very center and the very edges for a cleaner look
    float vignette = smoothstep(0.0, 0.1, dist) * smoothstep(1.5, 0.5, dist);

    color = texColor * vec4(rainbow, 1.0) * vignette;
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
