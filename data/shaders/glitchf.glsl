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

float random(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
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
    vec2 uv = applyZoomRotation(TexCoord, vec2(0.5));

    float gliTexCoordhStrength = 0.05 + 0.03 * sin(time * 3.0);
    float gliTexCoordhLine = step(0.5 + 0.1 * sin(time * 10.0 * iFrequency), fract(uv.y * 10.0 + time * 5.0));
    float displacement = gliTexCoordhStrength * (random(uv + time) - 0.5);

    vec2 distortedUV = uv;
    distortedUV.x += displacement * gliTexCoordhLine;

    vec4 texColor = texture(textTexture, distortedUV);

    vec2 offset = vec2(0.01 * sin(time * 15.0), 0.01 * cos(time * 10.0));
    vec3 gliTexCoordhRGB = vec3(
        texture(textTexture, distortedUV + vec2(offset.x, 0.0)).r,
        texture(textTexture, distortedUV + vec2(-offset.x, offset.y)).g,
        texture(textTexture, distortedUV + vec2(0.0, -offset.y)).b
    );

    float strobe = step(0.5, fract(time * 10.0)) * 0.2 * iAmplitude;

    FragColor = vec4(mix(texColor.rgb, gliTexCoordhRGB, gliTexCoordhLine + strobe), texColor.a);
}

