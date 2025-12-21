#version 300 es
precision highp float;

in vec2 TexCoord;
out vec4 FragColor;
uniform float time_f;
uniform sampler2D textTexture;
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

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

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
    float time_z = pingPong(time, 4.0) + 0.5;
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    vec2 normPos = uv * 2.0 - 1.0 * time_z;
    float dist = length(normPos);
    float phase = sin(dist * 10.0 * iFrequency - time * 4.0);
    vec2 TexCoordAdjusted = TexCoord + (normPos * 0.305 * phase);
    float dispersionScale = 0.02;
    vec2 dispersionOffset = normPos * dist * dispersionScale;
    vec2 TexCoordAdjustedR = TexCoordAdjusted + dispersionOffset * (-1.0);
    vec2 TexCoordAdjustedG = TexCoordAdjusted;
    vec2 TexCoordAdjustedB = TexCoordAdjusted + dispersionOffset * 1.0;
    float r = texture(textTexture, TexCoordAdjustedR).r;
    float g = texture(textTexture, TexCoordAdjustedG).g;
    float b = texture(textTexture, TexCoordAdjustedB).b;
    vec3 texColor = vec3(r, g, b);
    float angle = atan(normPos.y, normPos.x);
    float spiral = angle + time * 1.0;
    float hue = mod(spiral / (2.0 * 3.14159265), 1.0);
    vec3 rainbowColor = hsv2rgb(vec3(hue, 1.0, 1.0));
    vec3 finalColor = texColor * rainbowColor;
    float time_t = pingPong(time, 8.0) + 2.0;
    vec3 col = applyColorAdjustments(sin(finalColor * time_t));
    FragColor = vec4(col, 1.0);
}

