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
uniform float alpha_r;
uniform float alpha_g;
uniform float alpha_b;

vec4 xor_RGB(vec4 a, vec4 b) {
    ivec3 ai;
    ivec4 bi = ivec4(clamp(b, 0.0, 1.0) * 255.0 + 0.5);
    for (int i = 0; i < 3; ++i) {
        ai[i] = int(clamp(a[i], 0.0, 1.0) * 255.0 + 0.5);
        ai[i] = ai[i] ^ bi[i];
        ai[i] = ai[i] % 256;
        a[i] = float(ai[i]) / 255.0;
    }
    a.a = 1.0;
    return a;
}

vec4 blur(sampler2D image, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    highp float k[100];
    highp float v[100] = float[](
        0.5,1.0,1.5,2.0,2.5,2.5,2.0,1.5,1.0,0.5,
        1.0,2.0,2.5,3.0,3.5,3.5,3.0,2.5,2.0,1.0,
        1.5,2.5,3.0,3.5,4.0,4.0,3.5,3.0,2.5,1.5,
        2.0,3.0,3.5,4.0,4.5,4.5,4.0,3.5,3.0,2.0,
        2.5,3.5,4.0,4.5,5.0,5.0,4.5,4.0,3.5,2.5,
        2.5,3.5,4.0,4.5,5.0,5.0,4.5,4.0,3.5,2.5,
        2.0,3.0,3.5,4.0,4.5,4.5,4.0,3.5,3.0,2.0,
        1.5,2.5,3.0,3.5,4.0,4.0,3.5,3.0,2.5,1.5,
        1.0,2.0,2.5,3.0,3.5,3.5,3.0,2.5,2.0,1.0,
        0.5,1.0,1.5,2.0,2.5,2.5,2.0,1.5,1.0,0.5
    );
    for (int i = 0; i < 100; ++i) k[i] = v[i];
    float s = 0.0;
    for (int i = 0; i < 100; ++i) s += k[i];
    vec4 r = vec4(0.0);
    for (int y = -5; y <= 4; ++y) {
        for (int x = -5; x <= 4; ++x) {
            int idx = (y + 5) * 10 + (x + 5);
            r += texture(image, uv + vec2(float(x), float(y)) * ts) * k[idx];
        }
    }
    return r / s;
}

float pingPong(float x, float len) {
    float m = mod(x, len * 2.0);
    return m <= len ? m : (len * 2.0 - m);
}

void mxCacheShaderMain() {
    vec3 col = blur(samp, tc, iResolution).rgb;

    float t = mod(time_f, 6.0);
    float mcv = 1.0;

    if (t < 1.0) {
        col.r = mix(0.0, mcv, t);
    } else if (t < 2.0) {
        col.r = mcv;
        col.g = mix(0.0, mcv, t - 1.0);
    } else if (t < 3.0) {
        col.r = mcv;
        col.g = mcv;
        col.b = mix(0.0, mcv, t - 2.0);
    } else if (t < 4.0) {
        col = vec3(mcv);
        col.b = mix(mcv, alpha_b, t - 3.0);
    } else if (t < 5.0) {
        col = vec3(mcv, mcv, alpha_b);
        col.g = mix(mcv, alpha_g, t - 4.0);
    } else {
        col = vec3(mcv, alpha_g, alpha_b);
        col.r = mix(mcv, alpha_r, t - 5.0);
    }

    vec4 cyc = vec4(col, 1.0);
    vec4 xr = xor_RGB(blur(samp, tc, iResolution), cyc);

    float tt = pingPong(time_f, 20.0) + 2.0;
    vec3 s1 = 0.5 + 0.5 * sin(xr.rgb * tt);
    vec3 s2 = 0.5 + 0.5 * sin((xr.rgb * 6.28318) + vec3(0.0, 2.0943951, 4.1887902));

    vec3 allColors = mix(s1, s2, 0.65);
    color = vec4(clamp(allColors, 0.0, 1.0), 1.0);
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
