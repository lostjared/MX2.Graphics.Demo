#version 300 es
precision highp float;

in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D textTexture;
uniform float time_f;
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


float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
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

void main(void) {
    float time = time_f * iSpeed;
    float time_t = pingPong(time, 10.0) + 2.0;
    vec2 offset = vec2(0.01, 0.01) * time_t;
    vec4 FragColor1 = texture(textTexture, TexCoord);
    vec4 FragColor2 = texture(textTexture, TexCoord - offset);
    vec4 FragColor3 = texture(textTexture, TexCoord - offset * 2.0);
    vec4 FragColor4 = texture(textTexture, TexCoord - offset * 3.0);
    float weight1 = 0.5 * (1.0 - time_t);
    float weight2 = 0.3 * time_t;
    float weight3 = 0.15 * time_t;
    float weight4 = 0.05 * time_t;
    float totalWeight = weight1 + weight2 + weight3 + weight4;
    FragColor = (FragColor1 * weight1 + FragColor2 * weight2 + FragColor3 * weight3 + FragColor4 * weight4) / sin(totalWeight * time);
    
}
