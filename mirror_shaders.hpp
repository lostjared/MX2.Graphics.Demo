#ifndef __SHADERS_HPP_
#define __SHADERS_HPP_

#include"shaders.hpp"

inline const char *mirror_ufo_wrap = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;

uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

mat3 rotX(float a){float s=sin(a),c=cos(a);return mat3(1,0,0, 0,c,-s, 0,s,c);}
mat3 rotY(float a){float s=sin(a),c=cos(a);return mat3(c,0,s, 0,1,0, -s,0,c);}
mat3 rotZ(float a){float s=sin(a),c=cos(a);return mat3(c,-s,0, s,c,0, 0,0,1);}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = safeMirror(tc, textTexture);
    vec2 p2 = (uv - m) * ar;
    float ax = 0.25 * sin(time_f * 0.7);
    float ay = 0.25 * cos(time_f * 0.6);
    float az = time_f * 0.5;

    vec3 p3 = vec3(p2, 1.0);
    mat3 R = rotZ(az) * rotY(ay) * rotX(ax);
    vec3 r = R * p3;

    float k = 0.6;
    float zf = 1.0 / (1.0 + r.z * k);
    vec2 q = r.xy * zf;

    float dist = length(p2);
    float scale = 1.0 + 0.2 * sin(dist * 15.0 - time_f * 2.0);
    q *= scale;

    vec2 uvx = q / ar + m;
    uv = clamp(uvx, 0.0, 1.0);
    color = mxTexture(textTexture, uv);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_wrap_dmd6i = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;

const float PI = 3.1415926535897932384626433832795;

uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec2 grad_x = dFdx(uv);
    vec2 grad_y = dFdy(uv);
    vec3 s00 = textureGrad(img, wrapUV(uv + ts * vec2(-1.0, -1.0)), grad_x, grad_y).rgb;
    vec3 s10 = textureGrad(img, wrapUV(uv + ts * vec2(0.0, -1.0)), grad_x, grad_y).rgb;
    vec3 s20 = textureGrad(img, wrapUV(uv + ts * vec2(1.0, -1.0)), grad_x, grad_y).rgb;
    vec3 s01 = textureGrad(img, wrapUV(uv + ts * vec2(-1.0, 0.0)), grad_x, grad_y).rgb;
    vec3 s11 = textureGrad(img, wrapUV(uv), grad_x, grad_y).rgb;
    vec3 s21 = textureGrad(img, wrapUV(uv + ts * vec2(1.0, 0.0)), grad_x, grad_y).rgb;
    vec3 s02 = textureGrad(img, wrapUV(uv + ts * vec2(-1.0, 1.0)), grad_x, grad_y).rgb;
    vec3 s12 = textureGrad(img, wrapUV(uv + ts * vec2(0.0, 1.0)), grad_x, grad_y).rgb;
    vec3 s22 = textureGrad(img, wrapUV(uv + ts * vec2(1.0, 1.0)), grad_x, grad_y).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    vec4 baseTex = mxTexture(textTexture, tc);
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0); 
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(tc);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(tc, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((tc - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;

    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse *  PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_zoom = R"SHD1(#version 300 es
precision highp float;
precision highp int;

out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;

uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

mat3 rotX(float a){float s=sin(a),c=cos(a);return mat3(1,0,0, 0,c,-s, 0,s,c);}
mat3 rotY(float a){float s=sin(a),c=cos(a);return mat3(c,0,s, 0,1,0, -s,0,c);}
mat3 rotZ(float a){float s=sin(a),c=cos(a);return mat3(c,-s,0, s,c,0, 0,0,1);}

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

void main(void){
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));


			const float PI = 3.1415926535897932384626433832795;

    float aspect=iResolution.x/iResolution.y;
    vec2 ar=vec2(aspect,1.0);
    vec2 m=(iMouse.z>0.5)?(iMouse.xy/iResolution):vec2(0.5);

    vec2 p=(tc-m)*ar;
    vec3 v=vec3(p,1.0);
    float ax=0.25*sin(time_f*0.7);
    float ay=0.25*cos(time_f*0.6);
    float az=0.4*time_f;
    mat3 R=rotZ(az)*rotY(ay)*rotX(ax);
    vec3 r=R*v;
    float persp=0.6;
    float zf=1.0/(1.0+r.z*persp);
    vec2 q=r.xy*zf;

    float eps=1e-6;
    float base=1.72;
    float period=log(base);
    float t=time_f*0.5;
    float rad=length(q)+eps;
    float ang=atan(q.y,q.x)+t*0.3;
    float k=fract((log(rad)-t)/period);
    float rw=exp(k*period);
    vec2 qwrap=vec2(cos(ang),sin(ang))*rw;

    float N=8.0;
    float stepA=6.28318530718/N;
    float a=atan(qwrap.y,qwrap.x)+time_f*0.05;
    a=mod(a,stepA);
    a=abs(a-stepA*0.5);
    vec2 kdir=vec2(cos(a),sin(a));
    vec2 kaleido=kdir*length(qwrap);
    vec2 km = mod(kaleido, 2.0);
    kaleido = mix(km, 2.0 - km, step(1.0, km));
    vec2 uv=kaleido/ar+m;
    uv=mirrorCoord(uv);
    color=mxTexture(textTexture, mirrorCoord(sin(uv * (PI * pingPong(fract(time_f), 1.0))) * 0.5 + 0.5));

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_bubble = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    float len = length(uv);
    float time_t = pingPong(time_f, 10.0);
    float bubble = smoothstep(0.8, 1.0, 1.0 - len);
    bubble = sin(bubble * time_t);    
    vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
    distort = sin(distort * time_t);
    vec4 texColor = mxTexture(textTexture, distort * 0.5 + 0.5);
    color = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_geometric = R"SHD1(#version 300 es
precision highp float;
precision highp int;
in vec2 TexCoord;
out vec4 color;
uniform float time_f;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x,float length){
    float m=mod(x,length*2.0);
    return m<=length?m:length*2.0-m;
}

void main(void){
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    float A=clamp(amp,0.0,1.0);
    float U=clamp(uamp,0.0,1.0);
    float time_z=pingPong(time_f,4.0)+0.5;

    vec2 uv = safeMirror(tc, textTexture);
    vec2 center=(iMouse.z>0.5||iMouse.w>0.5)?(iMouse.xy/iResolution):vec2(0.5);
    vec2 normPos=(uv-center)*vec2(iResolution.x/iResolution.y,1.0);

    float dist=length(normPos);
    float phase=sin(dist*10.0-time_f*4.0);
    float phaseGain=mix(0.6,1.4,A);
    vec2 tcAdjusted=uv+(normPos*0.305*phase*phaseGain);

    float dispersionScale=0.02*(0.5+U);
    vec2 dispersionOffset=normPos*dist*dispersionScale;

    vec2 tcAdjustedR=tcAdjusted-dispersionOffset;
    vec2 tcAdjustedG=tcAdjusted;
    vec2 tcAdjustedB=tcAdjusted+dispersionOffset;

    float r=mxTexture(textTexture, mirrorCoord(tcAdjustedR)).r;
    float g=mxTexture(textTexture, mirrorCoord(tcAdjustedG)).g;
    float b=mxTexture(textTexture, mirrorCoord(tcAdjustedB)).b;

    color=vec4(r,g,b,1.0);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_prism = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    color = mxTexture(textTexture, uv);
    float radius = 1.0;
    vec2 center = iResolution * 0.5;
    vec2 texCoord = uv * iResolution;
    vec2 delta = texCoord - center;
    float dist = length(delta);
    float maxRadius = min(iResolution.x, iResolution.y) * radius;

    if (dist < maxRadius) {
        float scaleFactor = 1.0 - pow(dist / maxRadius, 2.0);
        vec2 direction = normalize(delta);
        float offsetR = 0.015;
        float offsetG = 0.0;
        float offsetB = -0.015;

        vec2 texCoordR = center + delta * scaleFactor + direction * offsetR * maxRadius;
        vec2 texCoordG = center + delta * scaleFactor + direction * offsetG * maxRadius;
        vec2 texCoordB = center + delta * scaleFactor + direction * offsetB * maxRadius;

        texCoordR /= iResolution;
        texCoordG /= iResolution;
        texCoordB /= iResolution;
        texCoordR = clamp(texCoordR, 0.0, 1.0);
        texCoordG = clamp(texCoordG, 0.0, 1.0);
        texCoordB = clamp(texCoordB, 0.0, 1.0);
        float r = mxTexture(textTexture, texCoordR).r;
        float g = mxTexture(textTexture, texCoordG).g;
        float b = mxTexture(textTexture, texCoordB).b;
        color = vec4(r, g, b, 1.0);
    } else {
        vec2 newTexCoord = mirrorCoord(tc);
        color = mxTexture(textTexture, newTexCoord);
    }

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_airshader1 = R"SHD1(#version 300 es
precision highp float;
precision highp int;
in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
uniform float seed;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float h1(float n){return fract(sin(n*91.345+37.12)*43758.5453123);}
vec2 h2(vec2 p){return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);}
vec2 rot(vec2 v,float a){float c=cos(a),s=sin(a);return vec2(c*v.x-s*v.y,s*v.x+c*v.y);}

void main(void){
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    float a=clamp(amp,0.0,1.0);
    float ua=clamp(uamp,0.0,1.0);
    float t=time_f;

    float speedR=5.0, ampR=0.03, waveR=10.0;
    float speedG=6.5, ampG=0.025, waveG=12.0;
    float speedB=4.0, ampB=0.035, waveB=8.0;

    float rR=sin(uv.x*waveR+t*speedR)*ampR + sin(uv.y*waveR*0.8+t*speedR*1.2)*ampR;
    float rG=sin(uv.x*waveG*1.5+t*speedG)*ampG + sin(uv.y*waveG*0.3+t*speedG*0.7)*ampG;
    float rB=sin(uv.x*waveB*0.5+t*speedB)*ampB + sin(uv.y*waveB*1.7+t*speedB*1.3)*ampB;

    vec2 tcR=uv+vec2(rR,rR);
    vec2 tcG=uv+vec2(rG,-0.5*rG);
    vec2 tcB=uv+vec2(0.3*rB,rB);

    vec3 pats[4]=vec3[](
        vec3(1.0,0.0,1.0),
        vec3(0.0,1.0,0.0),
        vec3(1.0,0.0,0.0),
        vec3(0.0,0.0,1.0)
    );
    float pspd=4.0;
    int pidx=int(mod(floor(t*pspd+seed*4.0),4.0));
    vec3 mir=pats[pidx];

    vec2 m = iMouse.z>0.5 ? (iMouse.xy/iResolution) : fract(vec2(0.37+0.11*sin(t*0.63+seed),0.42+0.13*cos(t*0.57+seed*2.0)));
    vec2 dR=tcR-m, dG=tcG-m, dB=tcB-m;

    float fallR=smoothstep(0.55,0.0,length(dR));
    float fallG=smoothstep(0.55,0.0,length(dG));
    float fallB=smoothstep(0.55,0.0,length(dB));

    float sw=(0.12+0.38*ua+0.25*a);
    vec2 tangR=rot(normalize(dR+1e-4),1.5707963);
    vec2 tangG=rot(normalize(dG+1e-4),1.5707963);
    vec2 tangB=rot(normalize(dB+1e-4),1.5707963);

    vec2 airR=tangR*sw*fallR*(0.06+0.22*a)*(0.6+0.4*cos(uv.y*40.0+t*3.0+seed));
    vec2 airG=tangG*sw*fallG*(0.06+0.22*a)*(0.6+0.4*cos(uv.y*38.0+t*3.3+seed*1.7));
    vec2 airB=tangB*sw*fallB*(0.06+0.22*a)*(0.6+0.4*cos(uv.y*42.0+t*2.9+seed*0.9));

    vec2 jit = (h2(uv*vec2(233.3,341.9)+t+seed)-0.5)*(0.0006+0.004*ua);
    tcR += airR + jit;
    tcG += airG + jit;
    tcB += airB + jit;

    vec2 fR=vec2(mir.r>0.5?1.0-tcR.x:tcR.x, tcR.y);
    vec2 fG=vec2(mir.g>0.5?1.0-tcG.x:tcG.x, tcG.y);
    vec2 fB=vec2(mir.b>0.5?1.0-tcB.x:tcB.x, tcB.y);

    float ca=0.0015+0.004*a;
    vec4 C=mxTexture(textTexture,uv);
    C.r=mxTexture(textTexture,fR+vec2( ca,0.0)).r;
    C.g=mxTexture(textTexture,fG               ).g;
    C.b=mxTexture(textTexture,fB+vec2(-ca,0.0)).b;

    float pulse=0.004*(0.5+0.5*sin(t*3.7+seed));
    C.rgb+=pulse*ua;

    color=vec4(C.rgb,1.0);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_pebble = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    vec2 normCoord = (uv * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
    float dist = length(normCoord);
    float maxRippleRadius = 25.0;
    float rippleSpeed = 2.0 * pingPong(time_f, 10.0);
    float phase = mod(time_f * rippleSpeed, maxRippleRadius);
    float ripple = sin((dist - phase) * 10.0) * exp(-dist * 3.0);
    vec2 displacedCoord = vec2(tc.x, tc.y + ripple * sin(time_f));
    color = mxTexture(textTexture, mirrorCoord(displacedCoord));

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_putty = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));


    vec2 uv = safeMirror(tc, textTexture);

    float timeVar = time_f * 0.5;
    vec2 noise = mirrorCoord(uv + timeVar);

    float stretchFactorX = 1.0 + 0.3 * sin(time_f + uv.y * 10.0);
    float stretchFactorY = 1.0 + 0.3 * cos(time_f + uv.x * 10.0);
    
    vec2 distortedUV = vec2(
        uv.x * stretchFactorX + noise.x * 0.1,
        uv.y * stretchFactorY + noise.y * 0.1
    );

    color = mxTexture(textTexture, mirrorCoord(distortedUV));

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_bowl_by_time = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

float h1(float n){ return fract(sin(n)*43758.5453123); }
vec2 h2(float n){ return fract(sin(vec2(n, n+1.0))*vec2(43758.5453,22578.1459)); }

void main(void){
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    float rate = 0.6;
    float t = time_f * rate;
    float t0 = floor(t);
    float a = fract(t);
    float w = a*a*(3.0-2.0*a);
    vec2 p0 = vec2(0.15) + h2(t0)*0.7;
    vec2 p1 = vec2(0.15) + h2(t0+1.0)*0.7;
    vec2 center = mix(p0, p1, w);
    vec2 uv = safeMirror(tc, textTexture);
    vec2 p = uv - center;
    float r = length(p);
    float ang = atan(p.y, p.x);
    float swirl = 2.2 + 0.8*sin(time_f*0.35);
    float spin = 0.6*sin(time_f*0.2);
    ang += swirl * r + spin;
    float bend = 0.35;
    float rp = r + bend * r * r;
    uv = center + vec2(cos(ang), sin(ang)) * rp;
    uv += 0.02 * vec2(sin((tc.y+time_f)*4.0), cos((tc.x-time_f)*3.5));
    uv = mirrorCoord(uv);
    color = mxTexture(textTexture, uv);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_comb_mouse = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;

uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec2 rotateUV(vec2 uv, float angle, vec2 center) {
    float s = sin(angle);
    float c = cos(angle);
    uv -= center;
    uv = mat2(c, -s, s, c) * uv;
    uv += center;
    return uv;
}

vec2 reflectUV(vec2 uv, float segments, vec2 center) {
    float angle = atan(uv.y - center.y, uv.x - center.x);
    float radius = length(uv - center);
    float segmentAngle = 2.0 * 3.14159265359 / segments;
    angle = mod(angle, segmentAngle);
    angle = abs(angle - segmentAngle * 0.5);
    return vec2(cos(angle), sin(angle)) * radius + center;
}

vec2 fractalZoom(vec2 uv, float zoom, float time, vec2 center) {
    for (int i = 0; i < 5; i++) {
        uv = abs((uv - center) * zoom) - 0.5 + center;
        uv = rotateUV(uv, sin(time * pingPong(time_f, 10.0)) * 0.1, center);
    }
    return uv;
}

void main() {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    uv = uv * iResolution / vec2(iResolution.y);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 center = m * iResolution / vec2(iResolution.y);

    vec4 originalTexture = mxTexture(textTexture, tc);
    vec2 kaleidoUV = reflectUV(uv, 6.0, center);
    float zoom = 1.5 + 0.5 * sin(time_f * 0.5);
    kaleidoUV = fractalZoom(kaleidoUV, zoom, time_f, center);
    kaleidoUV = rotateUV(kaleidoUV, time_f * 0.2, center);

    vec4 kaleidoColor = mxTexture(textTexture, kaleidoUV);
    float blendFactor = 0.6;
    vec4 blendedColor = mix(kaleidoColor, originalTexture, blendFactor);
    blendedColor.rgb *= 0.5 + 0.5 * sin(kaleidoUV.xyx + time_f);

    color = sin(blendedColor * pingPong(time_f, 10.0));
    vec4 t = mxTexture(textTexture, tc);
    color = color * t * 0.8;
    color = sin(color * pingPong(time_f, 15.0));
    color.a = 1.0;

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_grad = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec4 xor_RGB(vec4 icolor, vec4 source) {
    ivec3 int_color;
    ivec4 isource = ivec4(source * 255.0);
    for(int i = 0; i < 3; ++i) {
        int_color[i] = int(255.0 * icolor[i]);
        int_color[i] = int_color[i] ^ isource[i];
        if(int_color[i] > 255)
            int_color[i] = int_color[i] % 255;
        icolor[i] = float(int_color[i]) / 255.0;
    }
    icolor.a = 1.0;
    return icolor;
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    vec2 warp = uv + vec2(
        sin(uv.y * 10.0 + time_f) * 0.1,
        sin(uv.x * 20.0 + time_f) * 0.1
    );
    vec3 colorShift = vec3(
        0.2 * sin(time_f * 0.2) + 0.8,
        0.4 * sin(time_f * 0.4 + 6.0) + 0.5,
        0.8 * sin(time_f * 0.6 + 4.0) + 0.5
    );
    float feedback = rand(uv + time_f);
    vec2 feedbackUv = warp + feedback * 0.0;
    vec4 texColor = mxTexture(textTexture, feedbackUv);
    vec3 finalColor = texColor.rgb + colorShift;
    color = sin(vec4(finalColor, texColor.a) * (time_f * 0.5));
    color.a = 1.0;

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_mandella = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    float loopDuration = 25.0;
    float t = mod(time_f, loopDuration);
    vec2 aspect = vec2(iResolution.x / iResolution.y, 1.0);
    vec2 uv = safeMirror(tc, textTexture);
    vec2 nc = (uv * 2.0 - 1.0) * sin(aspect * time_f);
    nc.x = abs(nc.x);
    float d = length(nc);
    float a = atan(nc.y, nc.x);
    float spiralSpeed = 5.0;
    float inward = t / loopDuration;
    a += (1.0 - smoothstep(0.0, 8.0, d)) * t * spiralSpeed;
    d *= 1.0 - inward;
    vec2 spiral = vec2(cos(a), sin(a)) * tan(d);
    vec2 uv0 = (spiral / aspect + 1.0) * 0.5;

    vec2 p = (uv0 * 2.0 - 1.0) * aspect;
    float r = length(p);
    float ang = atan(p.y, p.x);

    float N = 10.0;
    float tau = 6.28318530718;
    float sector = tau / N;
    ang = mod(ang + 0.5 * sector, sector);
    ang = abs(ang - 0.5 * sector);

    float ringFreq = 6.0;
    float ring = fract(r * ringFreq + 0.15 * sin(time_f * 0.5));
    float ringMirror = abs(ring - 0.5) * 2.0;

    float swirl = 0.25 * sin(time_f * 0.3);
    ang += swirl * r;

    float zoom = 0.85 + 0.1 * sin(time_f * 0.27);
    vec2 m = vec2(cos(ang), sin(ang)) * (r * zoom * (0.85 + 0.15 * ringMirror));

    vec2 uvx = (m / aspect + 1.0) * 0.5;

    vec2 px = 1.0 / iResolution;
    vec3 c = mxTexture(textTexture, uv).rgb;
    c += mxTexture(textTexture, uvx + vec2(px.x, 0)).rgb;
    c += mxTexture(textTexture, uvx + vec2(-px.x, 0)).rgb;
    c += mxTexture(textTexture, uvx + vec2(0, px.y)).rgb;
    c += mxTexture(textTexture, uvx + vec2(0, -px.y)).rgb;
    c *= 0.2;

    float glow = smoothstep(0.95, 0.2, r) * (0.6 + 0.4 * ringMirror);
    vec3 base = c * (0.7 + 0.3 * glow);

    float hue = fract(ang / tau + 0.5 + 0.03 * time_f);
    float rC = clamp(abs(hue * 6.0 - 3.0) - 1.0, 0.0, 1.0);
    float gC = clamp(2.0 - abs(hue * 6.0 - 2.0), 0.0, 1.0);
    float bC = clamp(2.0 - abs(hue * 6.0 - 4.0), 0.0, 1.0);
    vec3 rainbow = vec3(rC, gC, bC);

    float rbAmt = 0.35 * smoothstep(0.15, 0.85, ringMirror);
    vec3 outCol = mix(base, base * rainbow, rbAmt);

    float vign = smoothstep(1.2, 0.4, length((tc - 0.5) * aspect));
    outCol *= vign;

    color = vec4(outCol, 1.0);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_air_bowl = R"SHD1(#version 300 es
precision highp float;
precision highp int;
in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
uniform float seed;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float h1(float n){return fract(sin(n*91.345+37.12)*43758.5453123);}
vec2 h2(vec2 p){return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);}
vec2 rot(vec2 v,float a){float c=cos(a),s=sin(a);return vec2(c*v.x-s*v.y,s*v.x+c*v.y);}

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void){
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    float a = clamp(amp,0.0,1.0);
    float ua = clamp(uamp,0.0,1.0);
    float t = time_f;

    vec2 center = vec2(0.5, 0.5);
    vec2 cv = safeMirror(tc, textTexture);
    vec2 baseUV = cv;
    vec2 offset = baseUV - center;
    float maxRadius = length(vec2(0.5, 0.5));
    float radius = length(offset);
    float normalizedRadius = radius / maxRadius;

    float distortion = (0.25 + 0.45*ua + 0.3*a);
    float distortedRadius = normalizedRadius + distortion * normalizedRadius * normalizedRadius;
    distortedRadius = clamp(distortedRadius, 0.0, 1.0);
    distortedRadius *= maxRadius;
    vec2 distortedCoords = center + distortedRadius * (radius > 0.0 ? offset / radius : vec2(0.0));

    float spinSpeed = 0.6 + 1.8*(0.3 + 0.7*a);
    float modulatedTime = pingPong(t * spinSpeed, 5.0);
    float angle = atan(distortedCoords.y - center.y, distortedCoords.x - center.x) + modulatedTime;

    vec2 rotatedTC;
    rotatedTC.x = cos(angle) * (distortedCoords.x - center.x) - sin(angle) * (distortedCoords.y - center.y) + center.x;
    rotatedTC.y = sin(angle) * (distortedCoords.x - center.x) + cos(angle) * (distortedCoords.y - center.y) + center.y;

    float warpAmp = 0.02 + 0.06*ua + 0.04*a;
    vec2 warpedCoords = mirrorCoord(rotatedTC + t * 0.12 * (1.0 + warpAmp*5.0));

    vec2 uv = cv * warpedCoords;

    float speedR=5.0, ampR=0.03, waveR=10.0;
    float speedG=6.5, ampG=0.025, waveG=12.0;
    float speedB=4.0, ampB=0.035, waveB=8.0;

    float rR=sin(uv.x*waveR+t*speedR)*ampR + sin(uv.y*waveR*0.8+t*speedR*1.2)*ampR;
    float rG=sin(uv.x*waveG*1.5+t*speedG)*ampG + sin(uv.y*waveG*0.3+t*speedG*0.7)*ampG;
    float rB=sin(uv.x*waveB*0.5+t*speedB)*ampB + sin(uv.y*waveB*1.7+t*speedB*1.3)*ampB;

    vec2 tcR=uv+vec2(rR,rR);
    vec2 tcG=uv+vec2(rG,-0.5*rG);
    vec2 tcB=uv+vec2(0.3*rB,rB);

    vec3 pats[4]=vec3[](vec3(1,0,1),vec3(0,1,0),vec3(1,0,0),vec3(0,0,1));
    float pspd=4.0;
    int pidx=int(mod(floor(t*pspd+seed*4.0),4.0));
    vec3 mir=pats[pidx];

    vec2 m = iMouse.z>0.5 ? (iMouse.xy/iResolution) : fract(vec2(0.37+0.11*sin(t*0.63+seed),0.42+0.13*cos(t*0.57+seed*2.0)));
    vec2 dR=tcR-m, dG=tcG-m, dB=tcB-m;

    float fallR=smoothstep(0.55,0.0,length(dR));
    float fallG=smoothstep(0.55,0.0,length(dG));
    float fallB=smoothstep(0.55,0.0,length(dB));

    float sw=(0.12+0.38*ua+0.25*a);
    vec2 tangR=rot(normalize(dR+1e-4),1.5707963);
    vec2 tangG=rot(normalize(dG+1e-4),1.5707963);
    vec2 tangB=rot(normalize(dB+1e-4),1.5707963);

    vec2 airR=tangR*sw*fallR*(0.06+0.22*a)*(0.6+0.4*cos(uv.y*40.0+t*3.0+seed));
    vec2 airG=tangG*sw*fallG*(0.06+0.22*a)*(0.6+0.4*cos(uv.y*38.0+t*3.3+seed*1.7));
    vec2 airB=tangB*sw*fallB*(0.06+0.22*a)*(0.6+0.4*cos(uv.y*42.0+t*2.9+seed*0.9));

    vec2 jit = (h2(uv*vec2(233.3,341.9)+t+seed)-0.5)*(0.0006+0.004*ua);
    tcR += airR + jit;
    tcG += airG + jit;
    tcB += airB + jit;

    vec2 fR=vec2(mir.r>0.5?1.0-tcR.x:tcR.x, tcR.y);
    vec2 fG=vec2(mir.g>0.5?1.0-tcG.x:tcG.x, tcG.y);
    vec2 fB=vec2(mir.b>0.5?1.0-tcB.x:tcB.x, tcB.y);

    float ca=0.0015+0.004*a;
    vec4 C=mxTexture(textTexture,uv);
    C.r=mxTexture(textTexture,fR+vec2( ca,0)).r;
    C.g=mxTexture(textTexture,fG              ).g;
    C.b=mxTexture(textTexture,fB+vec2(-ca,0)).b;

    float pulse=0.004*(0.5+0.5*sin(t*3.7+seed));
    C.rgb+=pulse*ua;

    color=vec4(C.rgb,1.0);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_sin_osc = R"SHD1(#version 300 es
precision highp float;
precision highp int;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
out vec4 color;

const float PI = 3.1415926535897932384626433832795;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

void main() {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    vec2 centeredUV = uv * 2.0 - 1.0;
    float angle = atan(centeredUV.y, centeredUV.x);
    float radius = length(centeredUV);
    float spin = time_f * 0.5;
    angle += floor(mod(angle + 3.14159, 3.14159 / 4.0)) * spin;
    angle = angle * pingPong(sin(time_f), 30.0);
    vec2 rotatedUV = vec2(cos(angle), sin(angle)) * radius;
    rotatedUV = abs(mod(rotatedUV, 2.0) - 1.0);
    vec4 texColor = mxTexture(textTexture, rotatedUV * 0.5 + 0.5);
    vec3 gradientEffect = vec3(
        0.5 + 0.5 * sin(time_f + uv.x * 15.0),
        0.5 + 0.5 * cos(time_f + uv.y * 15.0),
        0.5 + 0.5 * sin(time_f + (uv.x + uv.y) * 15.0)
    );
    vec3 finalColor = mix(texColor.rgb, gradientEffect, 0.3);
    color = vec4(finalColor, texColor.a);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_bubble_zoom_mouse = R"SHD1(#version 300 es
precision highp float;
precision highp int;

out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;

uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

mat3 rotX(float a){float s=sin(a),c=cos(a);return mat3(1,0,0, 0,c,-s, 0,s,c);}
mat3 rotY(float a){float s=sin(a),c=cos(a);return mat3(c,0,s, 0,1,0, -s,0,c);}
mat3 rotZ(float a){float s=sin(a),c=cos(a);return mat3(c,-s,0, s,c,0, 0,0,1);}

void main(void){
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    float aspect=iResolution.x/iResolution.y;
    vec2 ar=vec2(aspect,1.0);
    vec2 m=(iMouse.z>0.5)?(iMouse.xy/iResolution):vec2(0.5);

    vec2 cv = safeMirror(tc, textTexture);
    vec2 p=(cv-m)*ar;
    vec3 v=vec3(p,1.0);
    float ax=0.25*sin(time_f*0.7);
    float ay=0.25*cos(time_f*0.6);
    float az=0.4*time_f;
    mat3 R=rotZ(az)*rotY(ay)*rotX(ax);
    vec3 r=R*v;
    float persp=0.6;
    float zf=1.0/(1.0+r.z*persp);
    vec2 q=r.xy*zf;

    float eps=1e-6;
    float base=1.72;
    float period=log(base);
    float t=time_f*0.5;
    float rad=length(q)+eps;
    float ang=atan(q.y,q.x)+t*0.3;
    float k=fract((log(rad)-t)/period);
    float rw=exp(k*period);
    vec2 qwrap=vec2(cos(ang),sin(ang))*rw;

    float N=8.0;
    float stepA=6.28318530718/N;
    float a=atan(qwrap.y,qwrap.x)+time_f*0.05;
    a=mod(a,stepA);
    a=abs(a-stepA*0.5);
    vec2 kdir=vec2(cos(a),sin(a));
    vec2 kaleido=kdir*length(qwrap);

    vec2 uv=kaleido/ar+m;
    uv=mirrorCoord(uv);
    color=mxTexture(textTexture,uv);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_goofy = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 center = vec2(0.5);
    vec2 uv = safeMirror(tc, textTexture);
    uv = uv - center;
    float r = length(uv);
    float t = time_f;
    float s = pingPong(t, 10.0) * 0.1;

    float bendR = 0.15 + 0.1*sin(t*0.5);
    float swirl = (0.35 + 0.25*sin(t*0.33)) * (1.0 - smoothstep(0.0, 0.707, r));
    float ang = atan(uv.y, uv.x) + swirl;
    float rb = r * (1.0 + bendR * sin(r*12.0 + t*1.7));

    vec2 n1 = vec2(cos(t*0.37), sin(t*0.37));
    vec2 n2 = vec2(cos(t*0.53+1.7), sin(t*0.53+1.7));
    float w1 = sin(dot(uv, n1)*18.0 + t*1.3);
    float w2 = sin(dot(uv, n2)*14.0 - t*1.1);
    vec2 dirBend = normalize(n1)*w1 + normalize(n2)*w2;

    vec2 uvb = vec2(cos(ang), sin(ang)) * rb;
    uvb += dirBend * (0.025 + 0.02*sin(t*0.21)) * (0.5 + 0.5*sin(r*10.0 + t));

    float rot = sin(t*3.14159265*0.2) * 0.6;
    mat2 R = mat2(cos(rot), -sin(rot), sin(rot), cos(rot));
    uvb = R * uvb;

    uv = uvb + center;
    uv -= sin(uv*6.28318 + t) * (0.01 + 0.01*s);

    color = mxTexture(textTexture, uv);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_twist_gpt = R"SHD1(#version 300 es
precision highp float;
precision highp int;
in vec2 TexCoord;
out vec4 color;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pp(float x, float l){float m=mod(x,l*2.0);return abs(m-l);}
float s2(float x){return x*x*(3.0-2.0*x);}

void main(void){
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    float a = clamp(amp,0.0,1.0);
    float ua = clamp(uamp,0.0,1.0);

    vec2 c;
    if(iMouse.z>0.5) c = iMouse.xy / iResolution;
    else{
        float t = time_f*0.3;
        c = vec2(0.5) + 0.25*vec2(cos(t*0.9), sin(t*1.1))*0.6;
    }

    float r = length(uv - c);
    float rippleSpeed = mix(4.0,10.0,ua);
    float rippleAmp = mix(0.01,0.06,a);
    float rippleWave = mix(8.0,18.0,ua);
    float ripple = sin(uv.x*rippleWave + time_f*rippleSpeed) * rippleAmp;
    ripple += sin(uv.y*rippleWave*0.9 + time_f*rippleSpeed*1.07) * rippleAmp;
    vec2 uv_rip = uv + vec2(ripple);

    float twist = mix(0.8,2.2,a) * (r - 0.8) + time_f*(0.3+ua*0.7);
    float ca = cos(twist), sa = sin(twist);
    mat2 rot = mat2(ca,-sa,sa,ca);
    vec2 uv_twist = (rot*(uv - c)) + c;

    float w = s2(clamp(1.0 - r*2.0, 0.0, 1.0));
    float k = mix(0.35,0.75,ua);
    vec2 uv_mix = mix(uv_rip, uv_twist, k*w + (1.0-k)*0.5);

    float zoom = 1.0 + 0.02*sin(time_f*0.8 + ua*2.0);
    vec2 zc = (uv_mix - c)/zoom + c;

    vec4 col0 = mxTexture(textTexture, uv);
    vec4 col1 = mxTexture(textTexture, zc);
    //color = mix(col0, col1, 0.7);
    color = col1;

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_psyce_wave_all = R"SHD1(#version 300 es
precision highp float;
precision highp int;
in vec2 TexCoord;
out vec4 color;
uniform float time_f;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

void main(void)
{
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));


    vec2 uv = safeMirror(tc, textTexture);
    vec2 normCoord = ((tc.xy / iResolution.xy) * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);

    float distanceFromCenter = length(normCoord);
    float wave = sin(distanceFromCenter * 12.0 - time_f * 4.0);

    vec2 tcAdjusted = uv + (normCoord * 0.301 * wave);

    vec4 textureColor = mxTexture(textTexture, mirrorCoord(tcAdjusted));
    color = textureColor;

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_color_swirl = R"SHD1(#version 300 es
precision highp float;
precision highp int;

out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform float alpha;
uniform vec4 iMouse;

uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 m = (iMouse.z > 0.5 ? iMouse.xy : 0.5 * iResolution) / iResolution;
    vec2 uv = safeMirror(tc, textTexture);
    uv = (uv - m) * vec2(iResolution.x / iResolution.y, 1.0);
    float t = time_f * 0.5;
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);
    angle += t;
    float radMod = pingPong(radius + t * 0.5, 0.5);
    float wave = sin(radius * 10.0 - t * 5.0) * 0.5 + 0.5;
    float r = sin(angle * 3.0 + radMod * 10.0 + wave * 6.2831);
    float g = sin(angle * 4.0 - radMod * 8.0  + wave * 4.1230);
    float b = sin(angle * 5.0 + radMod * 12.0 - wave * 3.4560);
    vec3 col = vec3(r, g, b) * 0.5 + 0.5;
    vec3 texColor = mxTexture(textTexture, tc).rgb;
    col = mix(col, texColor, 0.3);
    color = vec4(sin(col * pingPong(time_f, 8.0) + 2.0), alpha);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_color_o1 = R"SHD1(#version 300 es
precision highp float;
precision highp int;

out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform float alpha;
uniform vec4 iMouse;

uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 m = (iMouse.z > 0.5 ? iMouse.xy : 0.5 * iResolution) / iResolution;
    vec2 uv = safeMirror(tc, textTexture);
    uv = (uv - m) * vec2(iResolution.x / iResolution.y, 1.0);
    float t = time_f * 0.5;
    float radius = length(uv);
    float angle = atan(uv.y, uv.x) + t;
    float radMod = pingPong(radius + t * 0.5, 0.5);
    float wave = sin(radius * 10.0 - t * 5.0) * 0.5 + 0.5;
    float r = sin(angle * 3.0 + radMod * 10.0 + wave * 6.2831);
    float g = sin(angle * 4.0 - radMod * 8.0  + wave * 4.1230);
    float b = sin(angle * 5.0 + radMod * 12.0 - wave * 3.4560);
    vec3 col = vec3(r, g, b) * 0.5 + 0.5;
    vec3 texColor = mxTexture(textTexture, tc).rgb;
    col = mix(col, texColor, 0.3);
    color = vec4(col, alpha);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *color_o1 = R"SHD1(#version 300 es
precision highp float;
precision highp int;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(


)SHD1";

inline const char *mirror_color_mouse = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform float alpha;
uniform vec4 iMouse;

uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 cv = safeMirror(tc, textTexture);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 uv = (cv - m) * vec2(iResolution.x / iResolution.y, 1.0);

    float t = time_f * 0.7;
    float beat = abs(sin(time_f * 3.14159)) * 0.2 + 0.8;

    float radius = length(uv);
    float angle = atan(uv.y, uv.x);
    angle += t * 0.5;
    float radMod = pingPong(radius + t * 0.3, 0.5);
    float wave = sin(radius * 10.0 - t * 6.0) * 0.5 + 0.5;

    float distortion = sin((radius + t * 0.5) * 8.0) * beat * 0.1;
    vec2 dir = vec2(cos(angle), sin(angle));
    vec2 tcSample = tc + dir * distortion;

    vec3 texColor = mxTexture(textTexture, tcSample).rgb;

    float noiseEffect = noise(uv * 10.0 + t * 0.5) * 0.2;
    float r = sin(angle * 3.0 + radMod * 8.0 + wave * 6.2831 + noiseEffect);
    float g = sin(angle * 4.0 - radMod * 6.0 + wave * 4.1230 + noiseEffect);
    float b = sin(angle * 5.0 + radMod * 10.0 - wave * 3.4560 - noiseEffect);
    vec3 col = vec3(r, g, b) * 0.5 + 0.5;

    col = mix(col, texColor, 0.6);

    vec3 lightDir = normalize(vec3(0.5, 0.5, 1.0));
    float d = dot(uv, uv);
    float z = sqrt(max(0.0, 1.0 - d));
    vec3 norm = normalize(vec3(uv, z));
    float light = dot(norm, lightDir) * 0.5 + 0.5;
    col *= light * 1.2;

    col *= beat;
    color = vec4(col, alpha);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_swirly = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;

uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 d = tc - m;
    float r = length(d);
    vec2 dir = d / max(r, 1e-6);

    float waveLength = 0.05;
    float amplitude = 0.02;
    float speed = 2.0;

    float ripple = sin((r / waveLength - time_f * speed) * 6.2831853);
    vec2 uv = safeMirror(tc, textTexture);
    uv = uv + dir * ripple * amplitude;
    color = mxTexture(textTexture, uv);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_pong2 = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

float h1(float n){ return fract(sin(n)*43758.5453123); }
vec2 h2(float n){ return fract(sin(vec2(n,n+1.0))*vec2(43758.5453,22578.1459)); }

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    float rate = 0.35;
    float t = time_f*rate;
    float k = floor(t);
    float a = fract(t);
    a = a*a*(3.0-2.0*a);

    vec2 p0 = h2(k)*2.0-1.0;
    vec2 p1 = h2(k+1.0)*2.0-1.0;
    vec2 shift = mix(p0,p1,a)*0.12;

    float s0 = 0.9 + 0.2*h1(k+10.0);
    float s1 = 0.9 + 0.2*h1(k+11.0);
    float scale = mix(s0,s1,a);

    float dir = sign(h1(k+20.0)-0.5);
    float modulatedTime = pingPong(time_f + h1(k+30.0)*2.0, 5.0);
    float rot = dir * modulatedTime;

    vec2 center = vec2(0.5) + shift;
    vec2 uv = safeMirror(tc, textTexture);
    vec2 d = uv - center;
    float ang = atan(d.y, d.x) + rot;
    float r = length(d)/scale;
    vec2 rotatedTC = center + r*vec2(cos(ang), sin(ang));
    rotatedTC = fract(rotatedTC);

    color = mxTexture(textTexture, mirrorCoord(rotatedTC));

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_pong = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    float angle1 = atan(uv.y - 0.5, uv.x - 0.5);
    float modulatedTime1 = pingPong(time_f, 3.0);
    angle1 += modulatedTime1;

    float angle2 = atan(uv.x - 0.5, uv.y - 0.5);
    float modulatedTime2 = pingPong(time_f * 0.5, 2.5);
    angle2 += modulatedTime2;

    float angle3 = atan(uv.y - 0.5 + modulatedTime2, uv.x - 0.5 + modulatedTime1);
    float modulatedTime3 = pingPong(time_f * 1.5, 4.0);
    angle3 += modulatedTime3;

    vec2 rotatedTC;
    rotatedTC.x = cos(angle3) * (uv.x - 0.5) - sin(angle3) * (uv.y - 0.5) + 0.5;
    rotatedTC.y = sin(angle3) * (uv.x - 0.5) + cos(angle3) * (uv.y - 0.5) + 0.5;
    
    rotatedTC = sin(rotatedTC * (modulatedTime1 * modulatedTime2 * modulatedTime3));

    color = mxTexture(textTexture, mirrorCoord(rotatedTC * 0.5 + 0.5));

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_spiral_aura = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

void main() {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    float angle = atan(uv.y, uv.x);
    float radius = length(uv);
    
    float spiral = angle + radius * 5.0 - time_f;
    float reflection = mod(spiral, 3.14159 * 2.0);
    
    vec2 spiralUV = vec2(cos(spiral), sin(spiral)) * radius + 0.5;
    vec2 reflectUV = vec2(cos(reflection), sin(reflection)) * radius + 0.5;

    vec4 gradientColor = vec4(0.5 + 0.5 * cos(time_f + radius * 10.0), 0.5 + 0.5 * sin(time_f + radius * 10.0), 0.5 + 0.5 * cos(time_f - radius * 10.0), 1.0);
    vec4 textureColor = mxTexture(textTexture, mirrorCoord(reflectUV));
    
    color = mix(gradientColor, textureColor, 0.5);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *spiral_aura = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

void main() {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = tc * 2.0 - 1.0;
    float angle = atan(uv.y, uv.x);
    float radius = length(uv);
    
    float spiral = angle + radius * 5.0 - time_f;
    float reflection = mod(spiral, 3.14159 * 2.0);
    
    vec2 spiralUV = vec2(cos(spiral), sin(spiral)) * radius + 0.5;
    vec2 reflectUV = vec2(cos(reflection), sin(reflection)) * radius + 0.5;

    vec4 gradientColor = vec4(0.5 + 0.5 * cos(time_f + radius * 10.0), 0.5 + 0.5 * sin(time_f + radius * 10.0), 0.5 + 0.5 * cos(time_f - radius * 10.0), 1.0);
    vec4 textureColor = mxTexture(textTexture, reflectUV);
    
    color = mix(gradientColor, textureColor, 0.5);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_center = R"SHD1(#version 300 es
precision highp float;
precision highp int;
in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture;
uniform float time_f;

uniform vec2 iResolution;
uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 center = vec2(0.5, 0.5);
    vec2 uv = safeMirror(tc, textTexture);
    float dist = length(uv);
    float angle = time_f * 2.0 + dist * 5.0;
    float s = sin(angle);
    float c = cos(angle);

    uv = vec2(
        uv.x * c - uv.y * s,
        uv.x * s + uv.y * c
    );
    
    uv += center;

    color = mxTexture(textTexture, uv);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_twisted = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    float angle = time_f * 0.5;
    float radius = length(uv - 0.5);
    float twist = radius * 5.0;
    float s = sin(twist + angle);
    float c = cos(twist + angle);
    float time_t = pingPong(time_f, 25.0) + 1.0;
    uv -= 0.5;
    uv = mat2(c, -s, s, c) * uv;
    uv += 0.5;
    // Apply seamless mirroring instead of manual flip
    uv = mirrorCoord(uv);
    color = mxTexture(textTexture, mirrorCoord(sin(uv * time_t) * 0.5 + 0.5));

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_gpt = R"SHD1(#version 300 es
precision highp float;
precision highp int;
in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

vec4 xor_RGB(vec4 icolor, vec4 source) {
    ivec3 int_color = ivec3(icolor.rgb * 255.0);
    ivec3 isource = ivec3(source.rgb * 255.0);
    for(int i = 0; i < 3; ++i) {
        int_color[i] = int_color[i] ^ isource[i];
        int_color[i] = int_color[i] % 256;
        icolor[i] = float(int_color[i]) / 255.0;
    }
    icolor.a = 1.0;
    return icolor;
}

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec4 enhancedBlur(sampler2D image, vec2 uv, vec2 resolution) {
    vec2 texelSize = 1.0 / resolution;
    vec4 result = vec4(0.0);
    float totalWeight = 0.0;

    int radius = 5;
    float fr = float(radius);
    for (int x = -radius; x <= radius; ++x) {
        for (int y = -radius; y <= radius; ++y) {
            vec2 iof = vec2(float(x), float(y));
            float distance = length(iof);
            float weight = exp(-(distance * distance) / (2.0 * fr * fr));
            vec2 offset = iof * texelSize;
            result += mxTexture(image, uv + offset) * weight;
            totalWeight += weight;
        }
    }
    return result / max(totalWeight, 1e-6);
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    uv.x += sin(uv.y * 20.0 + time_f * 2.0) * 0.005;
    uv.y += cos(uv.x * 20.0 + time_f * 2.0) * 0.005;
    vec4 tcolor = enhancedBlur(textTexture, uv, iResolution);
    float time_t = pingPong(time_f * 0.3, 5.0) + 1.0;
    vec4 modColor = vec4(
        tcolor.r * (0.5 + 0.5 * sin(time_f + uv.x * 10.0)),
        tcolor.g * (0.5 + 0.5 * sin(time_f + uv.y * 10.0)),
        tcolor.b * (0.5 + 0.5 * sin(time_f + (uv.x + uv.y) * 5.0)),
        tcolor.a
    );
    color = xor_RGB(sin(tcolor * time_f), modColor);
    color.rgb = pow(color.rgb, vec3(1.2));

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *mirror_bowl = R"SHD1(#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

float h1(float n){ return fract(sin(n)*43758.5453123); }
vec2 h2(float n){ return fract(sin(vec2(n, n+1.0))*vec2(43758.5453,22578.1459)); }

void main(void){
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    vec2 uv = safeMirror(tc, textTexture);
    
    float rate = 0.6;
    float t = time_f * rate;
    float t0 = floor(t);
    float a = fract(t);
    float w = a*a*(3.0-2.0*a);
    vec2 p0 = vec2(0.15) + h2(t0)*0.7;
    vec2 p1 = vec2(0.15) + h2(t0+1.0)*0.7;
    vec2 center = mix(p0, p1, w);

    vec2 p = uv - center;
    float r = length(p);
    float ang = atan(p.y, p.x);

    float swirl = 2.2 + 0.8*sin(time_f*0.35);
    float spin = 0.6*sin(time_f*0.2);
    ang += swirl * r + spin;

    float bend = 0.35;
    float rp = r + bend * r * r;

    vec2 uvx = center + vec2(cos(ang), sin(ang)) * rp;

    uvx += 0.02 * vec2(sin((tc.y+time_f)*4.0), cos((tc.x-time_f)*3.5));

    uvx = mirrorCoord(uvx);

    color = mxTexture(textTexture, uvx);

    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline const char *gpt_halluc = R"SHD1(#version 300 es
precision highp float;
precision highp int;

out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;

)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash(i + vec2(0.0, 0.0));
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 5; i++) {
        v += a * noise(p);
        p *= 2.1;
        a *= 0.55;
    }
    return v;
}

vec2 kaleido(vec2 p, float slices) {
    float pi = 3.14159265359;
    float r = length(p);
    float a = atan(p.y, p.x);
    float sector = pi * 2.0 / slices;
    a = mod(a, sector);
    a = abs(a - sector * 0.5);
    return vec2(cos(a), sin(a)) * r;
}

vec3 sampleWarp(vec2 uv, float t, float strength, vec2 center, vec2 res) {
    float aspect = res.x / res.y;

    float ampControl  = clamp(iAmplitude,  0.0, 2.0);
    float freqControl = clamp(iFrequency, 0.0, 2.0);

    vec2 p = (uv - center) * vec2(aspect, 1.0);
    float f1 = fbm(p * 1.8 + t * 0.3);
    float f2 = fbm(p.yx * 2.3 - t * 0.25);
    float f3 = fbm(p * 3.1 + vec2(1.3, -0.7) * t * 0.2);

    vec2 swirl = p;
    float r = length(swirl);
    float a = atan(swirl.y, swirl.x);
    a += (f1 * 4.0 + f2 * 2.0) * strength * 0.6;
    swirl = vec2(cos(a), sin(a)) * r;

    float sliceBase  = 8.0;
    float sliceRange = 8.0;
    float slices = sliceBase + sliceRange * (0.3 + 0.7 * ampControl) + 4.0 * sin(t * 0.17);

    vec2 k = kaleido(swirl + vec2(f2, f3) * 0.4 * strength, slices);

    vec2 flow = k;
    flow.x += f1 * 0.8 * strength;
    flow.y += (f2 - f3) * 0.8 * strength;

    vec2 base = flow / vec2(aspect, 1.0) + center;
    base = fract(base);

    float chromaBoost = 0.5 + 0.5 * ampControl;
    vec2 chromaShift = 0.0035 * strength * chromaBoost *
                       vec2(sin(t + f1 * 6.0), cos(t * 1.3 + f2 * 6.0));

    float rC = mxTexture(textTexture, base + chromaShift).r;
    float gC = mxTexture(textTexture, base).g;
    float bC = mxTexture(textTexture, base - chromaShift).b;
    vec3 col = vec3(rC, gC, bC);

    float bright = 0.7 + 0.6 * f3 + 0.4 * sin(t * 0.6 + f1 * 3.0);
    bright *= (0.6 + 0.8 * freqControl);

    col *= bright;

    float sat = 1.3 + 0.7 * sin(t * 0.43 + f2 * 5.0);
    sat *= (0.6 + 0.8 * freqControl);

    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(gray), col, sat);

    return col;
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));

    float ampControl  = clamp(iAmplitude,  0.0, 2.0);
    float freqControl = clamp(iFrequency, 0.0, 2.0);

    float tSpeed   = 0.3 + 1.7 * (freqControl * 0.5);
    float t        = time_f * tSpeed;
    float strength = 0.6 + 1.6 * (ampControl * 0.5);

    vec2 center = vec2(0.5);
    if (iMouse.z > 0.0) {
        center = iMouse.xy / iResolution;
    }

    vec3 colA = sampleWarp(tc, t, strength, center, iResolution);
    vec3 colB = sampleWarp(tc + vec2(0.01, -0.007),
                           t + 3.14,
                           strength * 0.9,
                           center,
                           iResolution);

    float blend = 0.5 + 0.5 * sin(t * 0.25);
    vec3 col = mix(colA, colB, blend);

    color.rgb = applyColorAdjustments(col);
    color.a = 1.0;
}
)SHD1";


inline const char *mirror_atan = R"SHD1(#version 300 es
precision highp float;
precision highp int;

out vec4 color;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

uniform vec4 iMouse;
uniform float amp;
uniform float uamp;
)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 tc = applyZoomRotation(TexCoord, vec2(0.5));
    vec2 uv = safeMirror(tc, textTexture);
    vec2 center = vec2(0.5, 0.5);
    vec2 offset = uv - center;
    float maxRadius = length(vec2(0.5, 0.5));
    float radius = length(offset);
    float normalizedRadius = radius / maxRadius;
    float angle = atan(offset.y, offset.x);
    float distortion = 0.5;
    float distortedRadius = normalizedRadius + distortion * pow(normalizedRadius, 2.0);
    distortedRadius = min(distortedRadius, 1.0);
    distortedRadius *= maxRadius;
    vec2 distortedCoords = center + distortedRadius * vec2(cos(angle), sin(angle));
    float modulatedTime = pingPong(time_f, 5.0);
    angle += modulatedTime;
    vec2 rotatedTC;
    rotatedTC.x = cos(angle) * (distortedCoords.x - center.x) - sin(angle) * (distortedCoords.y - center.y) + center.x;
    rotatedTC.y = sin(angle) * (distortedCoords.x - center.x) + cos(angle) * (distortedCoords.y - center.y) + center.y;
    vec2 warpedCoords = mirrorCoord(rotatedTC + time_f * 0.1);
    color = mxTexture(textTexture, mirrorCoord(warpedCoords));
    color.rgb = applyColorAdjustments(color.rgb);
}
)SHD1";

inline static const char *szBlock = R"SHD1(#version 300 es
precision highp float;
precision highp int;

out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;

)SHD1" COMMON_UNIFORMS COLOR_HELPERS R"SHD1(

vec2 rotate2D(vec2 p, float a) {
    float c = cos(a);
    float s = sin(a);
    return mat2(c, -s, s, c) * p;
}

vec2 aspectCorrect(vec2 uv) {
    float ar = iResolution.x / iResolution.y;
    uv -= 0.5;
    uv.x *= ar;
    return uv;
}

vec2 aspectUncorrect(vec2 p) {
    float ar = iResolution.x / iResolution.y;
    p.x /= ar;
    return p + 0.5;
}

void main(void) {
    vec2 uv = TexCoord;
    float t = time_f * (0.25 + iSpeed * 1.5);
    vec2 g = uv - 0.5;
    g = rotate2D(g, iRotation);
    float z = max(iZoom, 0.001);
    g /= z;
    uv = g + 0.5;

    float splitX = 0.52;
    
    vec2 swirlCenter = vec2(0.30, 0.55);
    if (iMouse.z > 0.0) {
        swirlCenter = iMouse.xy / iResolution;
    }

    
    vec2 uvL = uv;
    vec2 pL = aspectCorrect(uvL - swirlCenter);

    float r  = length(pL);
    float ang = atan(pL.y, pL.x);

    float swirlAmount = (2.5 + iFrequency * 4.0) * exp(-r * 1.5);
    swirlAmount *= (0.4 + iAmplitude * 1.6);

    ang += swirlAmount * sin(t * 0.7 + r * 6.0);

    vec2 sw = vec2(cos(ang), sin(ang)) * (r + 0.05 * sin(r * 40.0 + t * 2.5));
    sw = aspectUncorrect(sw) + swirlCenter - 0.5;

    sw.y += 0.08 * sin(sw.x * 18.0 - t * 3.0);

    uvL = sw;

    vec2 uvR = uv;

    float horizonY = 0.48;
    float dy = uvR.y - horizonY;

    float waveAmp = 0.015 + 0.03 * iAmplitude;
    float waveFreq = 12.0 + iFrequency * 24.0;

    float wave = waveAmp * exp(-abs(dy) * 8.0) *
                 sin(uvR.x * waveFreq - t * 2.0);

    uvR.y += wave;

    // Blend a bit more motion near the split so it feels like a vortex boundary
    float splitMask = smoothstep(splitX - 0.12, splitX + 0.02, uvR.x);
    uvR.y += splitMask * 0.03 * sin(uvR.y * 30.0 + t * 3.5);

    // --- Pick which UV to use based on side ---
    float leftSide = step(uv.x, splitX);
    vec2 sampleUV = uvR * (1.0 - leftSide) + uvL * leftSide;

    // Tile a bit so extreme warping stays “filled”
    sampleUV = fract(sampleUV);

    vec4 texColor = mxTexture(textTexture, sampleUV);

    // Vertical glowing seam between worlds
    float seam = smoothstep(splitX - 0.004, splitX, uv.x)
               - smoothstep(splitX, splitX + 0.004, uv.x);
    vec3 seamCol = vec3(0.75, 0.95, 1.0);
    texColor.rgb = mix(texColor.rgb, seamCol, seam * 0.9);

    // Slight vignette to focus center
    vec2 v = uv - 0.5;
    float vig = smoothstep(0.8, 0.2, length(v));
    texColor.rgb *= mix(0.85, 1.1, vig);

    // Apply your global color controls
    texColor.rgb = applyColorAdjustments(texColor.rgb);
    color = texColor;
}
)SHD1";

#endif