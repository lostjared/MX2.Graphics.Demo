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
uniform float time_f;      // global time
uniform vec2 iResolution;  // resolution of the window

//--------------------------------------
// Utility functions
//--------------------------------------
float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

vec2 random2(vec2 st) {
    st = vec2(dot(st, vec2(127.1, 311.7)),
              dot(st, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(st) * 43758.5453123);
}

vec2 smoothRandom2(float t) {
    float t0 = floor(t);
    float t1 = t0 + 1.0;
    vec2 rand0 = random2(vec2(t0));
    vec2 rand1 = random2(vec2(t1));
    float mix_factor = fract(t);
    return mix(rand0, rand1, smoothstep(0.0, 1.0, mix_factor));
}

vec3 rainbow(float t) {
    t = fract(t);
    float r = abs(t * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(t * 6.0 - 2.0);
    float b = 2.0 - abs(t * 6.0 - 4.0);
    return clamp(vec3(r, g, b), 0.0, 1.0);
}

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return (modVal <= length) ? modVal : (length * 2.0 - modVal);
}

//--------------------------------------
// Basic blur (kept as-is)
//--------------------------------------
vec4 blur(sampler2D image, vec2 uv, vec2 resolution) {
    vec2 texelSize = 1.0 / resolution;
    vec4 result = vec4(0.0);
    highp float kernel[100];
    highp float kernelVals[100] = float[](
        0.5, 1.0, 1.5, 2.0, 2.5, 2.5, 2.0, 1.5, 1.0, 0.5,
        1.0, 2.0, 2.5, 3.0, 3.5, 3.5, 3.0, 2.5, 2.0, 1.0,
        1.5, 2.5, 3.0, 3.5, 4.0, 4.0, 3.5, 3.0, 2.5, 1.5,
        2.0, 3.0, 3.5, 4.0, 4.5, 4.5, 4.0, 3.5, 3.0, 2.0,
        2.5, 3.5, 4.0, 4.5, 5.0, 5.0, 4.5, 4.0, 3.5, 2.5,
        2.5, 3.5, 4.0, 4.5, 5.0, 5.0, 4.5, 4.0, 3.5, 2.5,
        2.0, 3.0, 3.5, 4.0, 4.5, 4.5, 4.0, 3.5, 3.0, 2.0,
        1.5, 2.5, 3.0, 3.5, 4.0, 4.0, 3.5, 3.0, 2.5, 1.5,
        1.0, 2.0, 2.5, 3.0, 3.5, 3.5, 3.0, 2.5, 2.0, 1.0,
        0.5, 1.0, 1.5, 2.0, 2.5, 2.5, 2.0, 1.5, 1.0, 0.5
    );

    for (int i = 0; i < 100; i++) {
        kernel[i] = kernelVals[i];
    }

    float kernelSum = 0.0;
    for (int i = 0; i < 100; i++) {
        kernelSum += kernel[i];
    }

    for (int x = -5; x <= 4; ++x) {
        for (int y = -5; y <= 4; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(image, uv + offset) * kernel[(y + 5) * 10 + (x + 5)];
        }
    }

    return result / kernelSum;
}

//--------------------------------------
// Fractal function – now with infinite zoom logic
//--------------------------------------
float fractal(vec2 uv, vec2 c)
{
    // We'll do a simple Julia-like iteration
    vec2 z = uv;
    const float maxIterations = 50.0;
    float iteration = 0.0;

    for (float i = 0.0; i < maxIterations; i++)
    {
        // z = z^2 + c
        vec2 z_sq = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y);
        z = z_sq + c;

        if (length(z) > 2.0) break;
        iteration += 1.0;
    }

    return iteration / maxIterations;
}

//--------------------------------------
// Get a zoom + offset for “infinite” travel
//--------------------------------------
vec2 getFractalUV(vec2 uv, float t)
{
    // How long each zoom cycle lasts
    float cycleDuration = 6.0;
    // Which "zoom cycle" are we in?
    float cycleIndex    = floor(t / cycleDuration);
    // Local time in [0..cycleDuration)
    float cycleTime     = fract(t / cycleDuration) * cycleDuration;

    // Exponential zoom factor
    float zoomSpeed     = 0.5; // tweak this for faster or slower zoom
    float zoom          = pow(1.3, cycleTime * zoomSpeed);

    // Optionally pick a “center” each cycle so we jump around
    // For simplicity, pick some pseudorandom center:
    //   random2( cycleIndex ) returns something in [-1,1]
    vec2 randOffset = random2(vec2(cycleIndex * 37.1234));
    // Scale it down so it doesn’t jump too far from origin
    randOffset *= 0.5;

    // Move uv so that we zoom around randOffset
    // Step 1: shift uv relative to center
    uv -= randOffset;
    // Step 2: apply zoom
    uv /= zoom;
    // Step 3: shift back
    uv += randOffset;

    return uv;
}

void mxCacheShaderMain()
{
    //--------------------------------------
    // Normalize uv to [-1,1], correct aspect
    //--------------------------------------
    vec2 uv = tc * 2.0 - 1.0;
    uv.y *= iResolution.y / iResolution.x;

    //--------------------------------------
    // We create an infinite zoom
    //--------------------------------------
    // Decide how we choose c (the constant in z = z^2 + c).
    // You can also animate c over time in some interesting way.
    // Here, let’s revolve it with sine/cosine, but also include
    // cycle-based shifting so we pick different fractal shapes:
    float cycleIndex = floor(time_f / 6.0);
    vec2 cShift  = random2(vec2(cycleIndex * 13.97)) * 1.0;
    vec2 c = vec2(sin(time_f * 0.7), cos(time_f * 0.9)) + cShift;

    // Apply infinite zoom to uv
    vec2 zoomedUV = getFractalUV(uv, time_f);

    // Evaluate fractal
    float fVal = fractal(zoomedUV, c);

    // Map fractal value onto a rainbow color
    vec3 fractalColor = rainbow(fVal);

    //--------------------------------------
    // Optional: sample blurred background
    //--------------------------------------
    vec4 blurred_color = blur(samp, tc, iResolution);

    //--------------------------------------
    // Blend fractal color with blurred texture
    //--------------------------------------
    float blendFactor = 0.5;
    vec3 blended_color = mix(blurred_color.rgb, fractalColor, blendFactor);

    //--------------------------------------
    // Just a little extra time-based effect:
    //--------------------------------------
    float time_t = pingPong(time_f, 15.0) + 1.0;
    color = vec4(sin(blended_color * time_t), blurred_color.a);
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
