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
const float PI = 3.1415926535897932384626433832795;

float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453123);}
mat2 rot(float a){float s=sin(a),c=cos(a);return mat2(c,-s,s,c);}

float sdEquilateralTriangle(vec2 p){
    const float k=1.7320508075688772;
    p.x=abs(p.x)-1.0;
    p.y=p.y+1.0/k;
    if(p.x+k*p.y>0.0)p=vec2(p.x-k*p.y,-k*p.x-p.y)/2.0;
    p.x-=clamp(p.x,-2.0,0.0);
    return -length(p)*sign(p.y);
}

float triTile(vec2 p, float scale, float edge){
    p*=scale;
    vec2 g=floor(p);
    vec2 f=fract(p)-0.5;
    f*=rot(0.15*(sin(time_f*0.6)+sin(dot(g,vec2(1.7,2.3)))));
    float d=abs(sdEquilateralTriangle(f*2.0));
    float m=smoothstep(edge,0.0,d);
    return m;
}

float triFractal(vec2 p){
    float t=0.0;
    float s=3.0;
    float e=0.18;
    for(int i=0;i<4;i++){
        t=max(t,triTile(p,s,e));
        p*=rot(0.35);
        s*=1.9;
        e*=0.8;
    }
    return clamp(t,0.0,1.0);
}

void mxCacheShaderMain(){
    vec2 uv=tc*2.0-1.0;
    uv.x*=iResolution.x/iResolution.y;

    float radius=mix(0.8,1.2,0.5+0.5*sin(time_f*1.3))*2.0;
    float r=length(uv);
    float glow=smoothstep(radius,radius-0.25,r);

    vec4 base=texture(samp,tc);

    vec3 pink=vec3(1.0,0.2,0.6);
    float pulse=0.5+0.5*sin(time_f*3.0);
    float auraAmp=1.4*pulse;

    float triMask=triFractal(uv*1.2+vec2(0.15*sin(time_f*0.7),0.12*cos(time_f*0.55)));
    float lines=pow(triMask,1.5);
    float flakes=smoothstep(0.6,1.0,triMask);

    vec3 aura=pink*(0.55+0.45*flakes)*glow*auraAmp;
    vec3 linesGlow=pink*(0.35+0.65*pulse)*lines*glow;

    vec3 col=base.rgb;
    col=mix(col, col + aura, glow*0.85);
    col+=linesGlow*0.75;
    color=vec4(col, base.a);
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
