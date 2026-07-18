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
uniform vec4 iMouse;

float pingPong(float x, float l){float m=mod(x,l*2.0);return m<=l?m:l*2.0-m;}
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);vec2 u=f*f*(3.0-2.0*f);float a=hash(i),b=hash(i+vec2(1,0)),c=hash(i+vec2(0,1)),d=hash(i+vec2(1,1));return mix(mix(a,b,u.x),mix(c,d,u.x),u.y);}
vec3 rainbow(float t){t=fract(t);float r=abs(t*6.0-3.0)-1.0;float g=2.0-abs(t*6.0-2.0);float b=2.0-abs(t*6.0-4.0);return clamp(vec3(r,g,b),0.0,1.0);}

vec2 kaleido(vec2 uv, vec2 c, float seg, vec2 ar){
    vec2 p=(uv-c)*ar;
    float ang=atan(p.y,p.x);
    float ra=length(p);
    float a=6.2831853/seg;
    ang=mod(ang,a);
    ang=abs(ang-a*0.5);
    p=vec2(cos(ang),sin(ang))*ra;
    return c+p/ar;
}

vec2 vortex(vec2 uv, vec2 c, float t){
    vec2 d=uv-c;
    float r=length(d);
    float a=atan(d.y,d.x);
    a+=sin(t*0.9+r*12.0)*0.6+0.3*sin(r*24.0-t*1.7);
    float z=0.15*sin(t*0.6+pow(r,0.6)*10.0);
    r=clamp(r*(1.0-z),0.0,1.0);
    return c+vec2(cos(a),sin(a))*r;
}

vec4 blur9(sampler2D img, vec2 uv, vec2 res){
    vec2 ts=1.0/res;
    vec4 s=vec4(0.0);
    s+=texture(img,uv+ts*vec2(-1,-1));
    s+=texture(img,uv+ts*vec2( 0,-1));
    s+=texture(img,uv+ts*vec2( 1,-1));
    s+=texture(img,uv+ts*vec2(-1, 0));
    s+=texture(img,uv+ts*vec2( 0, 0));
    s+=texture(img,uv+ts*vec2( 1, 0));
    s+=texture(img,uv+ts*vec2(-1, 1));
    s+=texture(img,uv+ts*vec2( 0, 1));
    s+=texture(img,uv+ts*vec2( 1, 1));
    return s/9.0;
}

float luma(vec3 c){return dot(c,vec3(0.299,0.587,0.114));}

void mxCacheShaderMain(){
    vec2 ar=vec2(1.0,iResolution.y/iResolution.x);
    vec2 m=(iMouse.z>0.5)?(iMouse.xy/iResolution):vec2(0.5);

    vec3 orig=texture(samp, tc).rgb;

    vec2 uv=tc;
    uv=kaleido(uv,m,6.0,ar);
    uv=vortex(uv,m,time_f);

    float r1=0.003+0.003*sin(time_f*1.1);
    vec2 dir=normalize(uv-m+1e-6);
    vec2 tw=dir*(0.004*(noise(uv*vec2(iResolution.x/iResolution.y,1.0)*5.0+time_f*0.7)-0.5));
    vec4 b=blur9(samp,uv,iResolution);
    vec4 cR=texture(samp, uv+dir*r1+tw);
    vec4 cG=texture(samp, uv+tw);
    vec4 cB=texture(samp, uv-dir*r1+tw);
    vec3 texRGB=mix(b.rgb, vec3(cR.r,cG.g,cB.b), 0.85);

    vec2 p=(uv-m)*ar;
    float rad=length(p);
    float ang=atan(p.y,p.x);
    float beat=0.5+0.5*sin(time_f*2.1);
    vec3 base=rainbow(ang/6.2831853+time_f*0.12);
    vec3 glow=base*smoothstep(0.85,0.0,rad)*beat;
    float strobe=0.5+0.5*sin(time_f*10.0+rad*40.0);
    float w=0.28+0.22*strobe;

    vec3 fractal=mix(texRGB, texRGB*rainbow(time_f*0.08+rad*2.0), w)+glow*0.7;

    float L=luma(orig);
    float gx=abs(dFdx(L));
    float gy=abs(dFdy(L));
    float edge=smoothstep(0.02,0.15,gx+gy);

    float radial=smoothstep(0.05,0.65,rad);
    float band=1.0-smoothstep(0.015,0.035,abs(fract(ang*3.0/3.14159+time_f*0.08)-0.5));
    float n=0.35+0.65*noise(tc*vec2(iResolution.x/iResolution.y,1.0)*3.0+time_f*0.4);
    float strength=clamp(0.55*radial+0.25*strobe,0.0,1.0);
    strength*=mix(1.0,0.4,edge);
    strength*=mix(1.0,0.6,band);
    strength=mix(strength,strength*n,0.6);

    vec3 screenMix=2.5-(1.0-fractal)*(1.0-orig);
    vec3 outRGB=mix(orig, screenMix, strength);

    float vign=1.0-smoothstep(0.8,1.08,length((tc-m)*ar));
    color=vec4(outRGB*vign,1.0);
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
