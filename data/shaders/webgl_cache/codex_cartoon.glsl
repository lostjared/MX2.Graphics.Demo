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
uniform float alpha_r;
uniform float alpha_g;
uniform float alpha_b;
uniform float alpha;
uniform vec4 optx;
uniform vec4 random_var;
uniform float alpha_value;
uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform sampler2D samp;
uniform vec4 iMouse;
uniform float value_alpha_r, value_alpha_g, value_alpha_b;
uniform float index_value;
uniform float time_f;
uniform vec2 iResolution;

uniform float restore_black;
uniform vec4 inc_valuex;
uniform vec4 inc_value;
uniform vec2 image_pos;

float luma(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

vec3 sampleClamp(vec2 uv) {
    return texture(samp, clamp(uv, vec2(0.0), vec2(1.0))).rgb;
}

void mxCacheShaderMain()
{
    vec2 res = max(iResolution, vec2(1.0));
    vec2 px = 1.0 / res;
    vec4 src = texture(samp, tc);
    vec2 mouseUV = (iMouse.z > 0.0) ? (iMouse.xy / res) : vec2(0.5);
    vec2 mouseP = mouseUV * 2.0 - 1.0;
    mouseP.x *= res.x / res.y;
    vec2 p = tc * 2.0 - 1.0;
    p.x *= res.x / res.y;
    float mouseFocus = smoothstep(1.35, 0.0, length(p - mouseP));

    vec3 soft = src.rgb * 4.0;
    soft += sampleClamp(tc + vec2( px.x,  0.0));
    soft += sampleClamp(tc + vec2(-px.x,  0.0));
    soft += sampleClamp(tc + vec2( 0.0,  px.y));
    soft += sampleClamp(tc + vec2( 0.0, -px.y));
    soft *= 0.125;

    float lum = luma(soft);
    float shade = 0.24;
    if (lum > 0.78) {
        shade = 1.08;
    } else if (lum > 0.55) {
        shade = 0.82;
    } else if (lum > 0.33) {
        shade = 0.56;
    }

    vec3 hue = soft / max(lum, 0.001);
    vec3 toon = hue * shade;
    toon = floor(clamp(toon, 0.0, 1.0) * 6.0 + 0.5) / 6.0;
    toon = mix(vec3(luma(toon)), toon, 1.35);
    toon += mouseFocus * 0.08;

    float tl = luma(sampleClamp(tc + px * vec2(-1.0, -1.0)));
    float t  = luma(sampleClamp(tc + px * vec2( 0.0, -1.0)));
    float tr = luma(sampleClamp(tc + px * vec2( 1.0, -1.0)));
    float l  = luma(sampleClamp(tc + px * vec2(-1.0,  0.0)));
    float r  = luma(sampleClamp(tc + px * vec2( 1.0,  0.0)));
    float bl = luma(sampleClamp(tc + px * vec2(-1.0,  1.0)));
    float b  = luma(sampleClamp(tc + px * vec2( 0.0,  1.0)));
    float br = luma(sampleClamp(tc + px * vec2( 1.0,  1.0)));
    float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
    float gy = -tl - 2.0 * t - tr + bl + 2.0 * b + br;
    float ink = smoothstep(0.18, 0.48, length(vec2(gx, gy)));

    float hatch = step(0.55, fract((gl_FragCoord.x + gl_FragCoord.y) * 0.22));
    float shadowHatch = hatch * smoothstep(0.46, 0.18, lum) * 0.10;
    float paper = (hash(floor(gl_FragCoord.xy * 0.5) + floor(time_f * 2.0)) - 0.5) * 0.035;
    paper += mouseFocus * 0.02;

    toon -= shadowHatch;
    toon += paper;
    toon = mix(toon, vec3(0.015, 0.012, 0.01), ink * 0.92);

    color = vec4(clamp(toon, 0.0, 1.0), src.a);
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
