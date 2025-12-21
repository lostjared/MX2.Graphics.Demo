#version 300 es
precision highp float;

in vec2 TexCoord;
out vec4 FragColor;
uniform float time_f;
uniform vec2 iResolution;
uniform sampler2D textTexture;
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

float PI = 3.1415926535897932384626433832795;


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
    vec2 uv = (TexCoord - 0.5) * 2.0;
    uv.x *= iResolution.x / iResolution.y;
    
    float plasma = 0.0;
    plasma += sin((uv.x + time) * 5.0);
    plasma += sin((uv.y + time) * 5.0);
    plasma += sin((uv.x + uv.y + time) * 5.0);
    plasma += cos(length(uv + time) * 10.0 * iFrequency);
    plasma *= 0.25;

    vec3 baseColor;
    baseColor.r = cos(plasma * PI + time * 0.2 * iAmplitude) * 0.5 + 0.5;
    baseColor.g = sin(plasma * PI + time * 0.2) * 0.5 + 0.5;
    baseColor.b = sin(plasma * PI + time * 0.4) * 0.5 + 0.5;

    float dispersion = 0.02;
    vec3 prismColor;
    prismColor.r = texture(textTexture, TexCoord + vec2(dispersion, 0.0)).r;
    prismColor.g = texture(textTexture, TexCoord).g;
    prismColor.b = texture(textTexture, TexCoord - vec2(dispersion, 0.0)).b;

    FragColor = vec4(mix(baseColor, prismColor, 0.6), 1.0);
}

