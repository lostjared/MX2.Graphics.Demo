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

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec4 blur5(sampler2D image, vec2 uv, vec2 res) {
    highp float k1[5] = float[](1.0, 4.0, 6.0, 4.0, 1.0);
    vec2 ts = 1.0 / res;
    vec4 s = vec4(0.0);
    float wsum = 0.0;
    for (int j = -2; j <= 2; ++j) {
        for (int i = -2; i <= 2; ++i) {
            float w = k1[i+2] * k1[j+2];
            s += texture(image, uv + vec2(float(i), float(j)) * ts) * w;
            wsum += w;
        }
    }
    return s / wsum;
}

vec2 random2(vec2 st) {
    st = vec2(dot(st, vec2(127.1, 311.7)),
              dot(st, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(st) * 43758.5453123);
}

vec2 smoothRandom2(float t) {
    float t0 = floor(t);
    float t1 = t0 + 1.0;
    vec2 r0 = random2(vec2(t0));
    vec2 r1 = random2(vec2(t1));
    float a = fract(t);
    a = a * a * (3.0 - 2.0 * a);
    return mix(r0, r1, a);
}

vec3 rainbow(float t) {
    t = fract(t);
    float r = abs(t * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(t * 6.0 - 2.0);
    float b = 2.0 - abs(t * 6.0 - 4.0);
    return clamp(vec3(r, g, b), 0.0, 1.0);
}

vec4 xor_RGB(vec4 a, vec4 b) {
    uvec3 ua = uvec3(clamp(floor(a.rgb * 255.0 + 0.5), 0.0, 255.0));
    uvec3 ub = uvec3(clamp(floor(b.rgb * 255.0 + 0.5), 0.0, 255.0));
    uvec3 ux = ua ^ ub;
    return vec4(vec3(ux) / 255.0, 1.0);
}

void mxCacheShaderMain() {
    vec2 ar = vec2(iResolution.x / iResolution.y, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);

    vec4 tcolor = blur5(samp, tc, iResolution);

    float tA = pingPong(time_f, 10.0) + 2.0;
    float tB = pingPong(time_f, 5.0) + 2.0;
    float tw = pingPong(time_f, 15.0) + 1.0;

    vec2 uvn = (tc - m) * ar;
    float wave = sin(uvn.x * 10.0 + tw * 2.0) * 0.1;
    vec2 rnd = smoothRandom2(tw) * 0.5;
    float expand = 0.5 + 0.5 * sin(tw * 2.0);
    vec2 suv = uvn * expand + rnd;
    float ang = atan(suv.y + wave, suv.x) + tw * 2.0;

    vec3 rb = rainbow(ang / 6.2831853);
    vec3 base = tcolor.rgb;
    vec3 mixc = mix(base, rb, 0.5);

    color = xor_RGB(sin(vec4(mixc, 1.0) * tA), tcolor * tB);
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
