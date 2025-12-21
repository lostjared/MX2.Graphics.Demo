#version 300 es
precision highp float;

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
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

const int SEGMENTS = 6;
float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec4 adjustHue(vec4 c, float angle) {
    float U = cos(angle);
    float W = sin(angle);
    mat3 rotationMatrix = mat3(
        0.299,  0.587,  0.114,
        0.299,  0.587,  0.114,
        0.299,  0.587,  0.114
    ) + mat3(
        0.701, -0.587, -0.114,
       -0.299,  0.413, -0.114,
       -0.300, -0.588,  0.886
    ) * U
      + mat3(
         0.168,  0.330, -0.497,
        -0.328,  0.035,  0.292,
         1.250, -1.050, -0.203
    ) * W;
    return vec4(rotationMatrix * c.rgb, c.a);
}


vec3 adjustBrightness(vec3 col, float b) {
    return col * b;
}

vec3 adjustContrast(vec3 col, float c) {
    return (col - 0.5) * c + 0.5;
}

vec3 adjustSaturation(vec3 col, float s) {
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    return mix(vec3(gray), col, s);
}

vec3 rotateHue(vec3 col, float angle) {
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

vec3 applyColorAdjustments(vec3 col) {
    col = adjustBrightness(col, iBrightness);
    col = adjustContrast(col, iContrast);
    col = adjustSaturation(col, iSaturation);
    col = rotateHue(col, iHueShift);
    return clamp(col, 0.0, 1.0);
}

vec2 applyZoomRotation(vec2 uv, vec2 center) {
    vec2 p = uv - center;
    float c = cos(iRotation);
    float s = sin(iRotation);
    p = mat2(c, -s, s, c) * p;
    float z = max(abs(iZoom), 0.001);
    p /= z;
    return p + center;
}

void main() {
    float time = time_f * iSpeed;
    vec2 uv = (TexCoord - 0.5) * iResolution / min(iResolution.x, iResolution.y);
    float r = length(uv);
    float angle = atan(uv.y, uv.x);
    float swirlStrength = 2.5;
    float swirl = time * 0.5;
    angle += swirlStrength * sin(swirl + r * 4.0);
    float segmentAngle = 2.0 * 3.14159265359 / float(SEGMENTS);
    angle = mod(angle, segmentAngle);
    angle = abs(angle - segmentAngle * 0.5);
    vec2 kaleidoUV = vec2(cos(angle), sin(angle)) * r;
    float ripple = sin(r * 12.0 - pingPong(time, 10.0) * 10.0 * iFrequency) * exp(-r * 4.0);
    kaleidoUV += ripple * 0.01 * iAmplitude;
    vec2 scale = vec2(1.0) / (iResolution / min(iResolution.x, iResolution.y));
    vec2 textTextureleTC = kaleidoUV * scale + 0.5;
    
    float offsetAmount = 0.003 * sin(time * 0.5);
    vec4 col = texture(textTexture, textTextureleTC);
    col += texture(textTexture, textTextureleTC + vec2(offsetAmount, 0.0));
    col += texture(textTexture, textTextureleTC + vec2(-offsetAmount, offsetAmount));
    col += texture(textTexture, textTextureleTC + vec2(offsetAmount * 0.5, -offsetAmount));
    col /= 4.0;
    
    float hueSpeed = 2.0;
    float hueShift = (time * hueSpeed + ripple * 2.0);
    
    FragColor = adjustHue(col, hueShift);
    float saturationBoost = 1.5;
    float brightnessBoost = 1.1;
    vec3 hsv = FragColor.rgb;
    float avg = (hsv.r + hsv.g + hsv.b) / 3.0;
    hsv = mix(vec3(avg), hsv, saturationBoost);
    hsv *= brightnessBoost;
    FragColor.rgb = hsv;
    vec4 mixResult = mix(clamp(FragColor, 0.0, 1.0), texture(textTexture, TexCoord), 0.5);
    float time_t = pingPong(time, 10.0) + 2.0;
    vec3 final_col = sin(mixResult.rgb * time_t);
    vec3 adjustedCol = applyColorAdjustments(final_col);
    FragColor = vec4(adjustedCol, mixResult.a);
}

