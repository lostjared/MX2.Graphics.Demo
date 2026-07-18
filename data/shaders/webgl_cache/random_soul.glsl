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

float pingPong(float x, float len){float m=mod(x,len*2.0);return m<=len?m:len*2.0-m;}
float h(float n){return fract(sin(n)*43758.5453123);}
vec2 h2(float n){return vec2(h(n),h(n+1.23));}

vec3 rainbow(float t){
    t=fract(t);
    float r=abs(t*6.0-3.0)-1.0;
    float g=2.0-abs(t*6.0-2.0);
    float b=2.0-abs(t*6.0-4.0);
    return clamp(vec3(r,g,b),0.0,1.0);
}

void mxCacheShaderMain(){
    vec2 res=iResolution;
    vec2 uv=tc*2.0-1.0;
    uv.y*=res.y/res.x;

    float t=time_f*0.25;
    float seg=floor(t);
    float a=fract(t);
    vec2 p0=-0.5+h2(seg)*1.0;
    vec2 p1=-0.5+h2(seg+1.0)*1.0;
    a=a*a*(3.0-2.0*a);
    vec2 swirlC=mix(p0,p1,a);

    vec2 d=uv-swirlC;
    float r=length(d);
    float k=0.85;
    float s=0.45;
    float theta=k*exp(-r*2.5)*(1.0+0.5*sin(time_f*0.6));
    float ct=cos(theta);
    float st=sin(theta);
    vec2 rot=vec2(d.x*ct-d.y*st,d.x*st+d.y*ct)+swirlC;

    float angle=atan(uv.y,uv.x)+time_f*20.0;
    vec3 rain=rainbow(angle/(2.0*3.1415926535));

    float bulge_strength=0.2;
    float distortion=pow(length(rot),2.0)*bulge_strength;
    vec2 distorted=rot*(1.0+distortion);

    vec2 tex=distorted*0.5+0.5;
    vec4 base=texture(samp,tex);
    vec3 blend=mix(base.rgb,rain,0.5);

    color=vec4(sin(blend*time_f),base.a);
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
