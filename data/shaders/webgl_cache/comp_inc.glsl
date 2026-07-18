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

vec3 compositeEffect(vec2 uv) {
    float offset = 0.01;
    vec3 col;
    col.r = texture(samp, uv + vec2(offset, 0.0)).r;
    col.g = texture(samp, uv).g;
    col.b = texture(samp, uv - vec2(offset, 0.0)).b;
    float noise = fract(sin(dot(uv.xy, vec2(12.9898, 78.233))) * 43758.5453);
    col += noise * 0.05;
    float scanline = sin(uv.y * iResolution.y * 1.5) * 0.1;
    col -= scanline;
    float bleed = sin(uv.y * iResolution.y * 0.2 + time_f * 5.0) * 0.005;
    col.r += bleed * 0.002;
    col.b -= bleed * 0.002;
    return col;
}

vec3 aces(vec3 x){
    const mat3 a=mat3(0.59719,0.35458,0.04823,0.07600,0.90834,0.01566,0.02840,0.13383,0.83777);
    const mat3 b=mat3(1.60475,-0.53108,-0.07367,-0.10208,1.10813,-0.00605,-0.00327,-0.07276,1.07602);
    vec3 v=a*x;
    v=(v*(v+0.0245786)-0.000090537)*b;
    return clamp(v,0.0,1.0);
}

float hash12(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453123);}

void mxCacheShaderMain() {
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect,1.0);
    vec2 c = vec2(0.5);

    vec2 p = (tc - c) * ar;
    float r2 = dot(p,p);
    float k1 = 0.15;
    float k2 = 0.05;
    vec2 pd = p*(1.0 + k1*r2 + k2*r2*r2);
    vec2 uvd = pd/ar + c;

    vec2 dir = normalize(p + 1e-6);
    float ab = 0.0015 + 0.0015*sin(time_f*0.5);
    vec2 uvR = uvd + dir*ab;
    vec2 uvG = uvd;
    vec2 uvB = uvd - dir*ab;

    uvR = clamp(uvR,0.0,1.0);
    uvG = clamp(uvG,0.0,1.0);
    uvB = clamp(uvB,0.0,1.0);

    vec3 cR = compositeEffect(uvR);
    vec3 cG = compositeEffect(uvG);
    vec3 cB = compositeEffect(uvB);
    vec3 col = vec3(cR.r, cG.g, cB.b);

    vec2 ts = 1.0 / iResolution;
    float shAmt = 0.4;
    vec3 bsum = vec3(0.0);
    bsum += compositeEffect(uvd + vec2(1,1)*ts);
    bsum += compositeEffect(uvd + vec2(-1,1)*ts);
    bsum += compositeEffect(uvd + vec2(1,-1)*ts);
    bsum += compositeEffect(uvd + vec2(-1,-1)*ts);
    bsum *= 0.25;
    col += (col - bsum) * shAmt;

    float v = smoothstep(0.95, 0.3, length(p));
    col *= v;

    float gAmt = 0.03;
    float g = hash12(gl_FragCoord.xy + time_f*123.45) - 0.5;
    col += g * gAmt;

    col = aces(col);
    col = pow(col, vec3(1.0/2.2));

    color = vec4(clamp(col,0.0,1.0),1.0);
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
