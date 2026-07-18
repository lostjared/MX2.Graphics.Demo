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

float hash21(vec2 p){p=fract(p*vec2(123.34,456.21));p+=dot(p,p+34.45);return fract(p.x*p.y);}
mat2 rot(float a){float c=cos(a),s=sin(a);return mat2(c,-s,s,c);}

void mxCacheShaderMain(){
    vec2 ar = vec2(iResolution.x/iResolution.y,1.0);
    vec2 m = (iMouse.z>0.5)? (iMouse.xy/iResolution) : vec2(0.5);
    vec2 p = (tc - m)*ar;
    float r = length(p);
    float base = mix(0.015,0.08,smoothstep(0.0,0.9,r));
    float wob = 0.012*sin(time_f*0.9+5.0*r);
    float ts = base + wob;
    vec2 gid = floor(p/ts);
    vec2 cellCenter = (gid+0.5)*ts;
    float ang = 0.6*sin(time_f*0.7 + hash21(gid)*6.2831);
    vec2 local = rot(ang)*(p-cellCenter);
    vec2 jitter = (hash21(gid+13.7)-0.5)*0.25*ts*vec2(cos(time_f+gid.x),sin(time_f*1.3+gid.y));
    vec2 uv = (cellCenter + jitter)/ar + m;
    vec4 tex = texture(samp, uv + local*0.35);
    vec2 edge = abs(fract(p/ts)-0.5);
    float border = smoothstep(0.01,0.0,min(edge.x,edge.y));
    vec3 tint = mix(tex.rgb, vec3(0.0,1.0,0.2), 0.25+0.25*sin(time_f+hash21(gid)*3.7));
    vec3 col = mix(vec3(0.08,0.12,0.08), tint, border);
    float vig = 1.0 - 0.65*smoothstep(0.7,1.1,length((tc-0.5)*vec2(ar.x,1.0)));
    color = vec4(col*vig,1.0);
    color =  mix(color, texture(samp, tc), 0.5);
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
