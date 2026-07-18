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

float h1(float n){return fract(sin(n)*43758.5453123);}
float h2(vec2 p){return fract(sin(dot(p,vec2(12.9898,78.233)))*43758.5453);}

vec4 xor_RGB(vec4 icolor, vec4 source){
    ivec3 ic; ivec4 isrc = ivec4(source*255.0);
    for(int i=0;i<3;++i){
        ic[i]=int(255.0*icolor[i]);
        ic[i]=ic[i]^isrc[i];
        if(ic[i]>255) ic[i]=ic[i]%255;
        icolor[i]=float(ic[i])/255.0;
    }
    icolor.a=1.0;
    return icolor;
}

vec3 fastBlurRot(sampler2D tex, vec2 uv, vec2 res, float t){
    float ang = t*2.3 + h1(floor(t*0.37))*6.2831853;
    vec2 dir = vec2(cos(ang), sin(ang));
    vec2 px = dir / res;
    vec3 a = texture(tex, uv - 4.0*px).rgb;
    vec3 b = texture(tex, uv - 2.5*px).rgb;
    vec3 c = texture(tex, uv - 1.5*px).rgb;
    vec3 d = texture(tex, uv - 0.5*px).rgb;
    vec3 e = texture(tex, uv + 0.5*px).rgb;
    vec3 f = texture(tex, uv + 1.5*px).rgb;
    vec3 g = texture(tex, uv + 2.5*px).rgb;
    vec3 h = texture(tex, uv + 4.0*px).rgb;
    vec3 m = texture(tex, uv).rgb;
    return (a*0.05+b*0.08+c*0.12+d*0.15+m*0.2+e*0.15+f*0.12+g*0.08+h*0.05);
}

float pingPong(float x,float l){
    float m=mod(x,l*2.0);
    return m<=l?m:l*2.0-m;
}

void mxCacheShaderMain(){
    float t=time_f;
    float speed = 2.0 + h1(floor(t*0.31))*2.5;
    float amp = 0.015 + 0.01*h1(floor(t*0.53));
    vec2 jitter = vec2(h2(tc*123.0+t), h2(tc*321.0-t))*amp*0.75;

    vec2 uv = tc + jitter;
    vec3 base = fastBlurRot(samp, uv, iResolution, t*speed);

    float cyc = mod(t*(1.0+0.4*h1(floor(t*0.19))), 6.0);
    vec3 gate = vec3(1.0);
    if(cyc<1.0){ gate.r = mix(0.0,1.0,cyc); gate.g=0.0; gate.b=0.0; }
    else if(cyc<2.0){ gate.r=1.0; gate.g=mix(0.0,1.0,cyc-1.0); gate.b=0.0; }
    else if(cyc<3.0){ gate.r=1.0; gate.g=1.0; gate.b=mix(0.0,1.0,cyc-2.0); }
    else if(cyc<4.0){ gate=vec3(1.0); gate.b=mix(1.0,alpha_b,cyc-3.0); }
    else if(cyc<5.0){ gate.r=1.0; gate.b=alpha_b; gate.g=mix(1.0,alpha_g,cyc-4.0); }
    else { gate.g=alpha_g; gate.b=alpha_b; gate.r=mix(1.0,alpha_r,cyc-5.0); }

    vec2 chrom = (vec2(cos(t*0.7), sin(t*0.9))*0.75 + vec2(0.3,-0.2))* (0.35+0.35*h1(floor(t*0.27)));
    chrom /= iResolution;
    vec3 chromSample;
    chromSample.r = texture(samp, uv + chrom*1.0).r;
    chromSample.g = texture(samp, uv).g;
    chromSample.b = texture(samp, uv - chrom*1.0).b;

    vec3 mixin = mix(base, chromSample, 0.35 + 0.25*h1(floor(t*0.41)));
    vec4 gateColor = vec4(gate,1.0);

    float tt = pingPong(t, 20.0) + 2.0;
    vec4 xored = xor_RGB(vec4(mixin,1.0), gateColor);
    vec4 s = sin(xored * tt);
    s.a = 1.0;
    color = s;
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
