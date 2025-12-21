#version 300 es
precision highp float;
out vec4 FragColor;
in vec2 TexCoord;

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

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
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
    float time_t = pingPong(time, 7.0);
    vec2 uv = applyZoomRotation(TexCoord, vec2(0.5)) * 2.0 - 1.0;
    float frequency = 10.0;
    float amplitude = 0.02;
    float waveX = sin(uv.y * frequency + time_t) * amplitude;
    waveX += sin(uv.y * frequency * 0.5 + time_t * 1.5) * amplitude * 0.5;
    float waveY = sin(uv.x * frequency + time_t * 0.8) * amplitude;
    waveY += sin(uv.x * frequency * 0.5 + time_t * 1.2) * amplitude * 0.5;
    uv += vec2(waveX, waveY);

    float hue = mod((uv.y + 1.0) * 0.5 + time_t * 0.1 * iAmplitude, 1.0);
    float saturation = 1.0;
    float value = 1.0;
    vec3 neonColor = hsv2rgb(vec3(hue, saturation, value));

    vec4 texColor = texture(textTexture, uv * 0.5 + 0.5);
    float depth = (uv.y + 1.0) * 0.5;
    float attenuation = mix(1.0, 0.6, depth);
    vec3 finalColor = mix(neonColor, sin(texColor.rgb * time_t), 0.5) * attenuation;

    vec3 col = applyColorAdjustments(finalColor);
    FragColor = vec4(col, 1.0);
}
