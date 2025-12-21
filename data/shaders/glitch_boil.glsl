#version 300 es
precision highp float;
out vec4 FragColor;
in vec2 TexCoord;

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

float random(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(random(i + vec2(0.0, 0.0)), random(i + vec2(1.0, 0.0)), u.x),
               mix(random(i + vec2(0.0, 1.0)), random(i + vec2(1.1, 1.0)), u.x), u.y);
}

float bubble(vec2 uv, vec2 position, float size, float time) {
    float dist = length(uv - position);
    float pop = smoothstep(size, size - 0.05, dist) * (1.0 - smoothstep(size - 0.05, size - 0.1, dist));
    return pop * (1.0 - smoothstep(0.0, 1.0, time));
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
    vec2 uv = applyZoomRotation(TexCoord, vec2(0.5));
    time = mod(time, 10.0);

    float boil = noise(uv * 10.0 * iFrequency + time * 2.0) * 0.1 * iAmplitude;
    boil += noise(uv * 15.0 + time * 3.0) * 0.05;
    boil += noise(uv * 20.0 + time * 5.0) * 0.02;

    vec2 distorted_uv = uv + boil * vec2(noise(uv + time), noise(uv + time + vec2(10.0, 10.0)));

    vec3 col = texture(textTexture, distorted_uv).rgb;

    float bubble1 = bubble(uv, vec2(0.5, 0.5), 0.1, time);
    float bubble2 = bubble(uv, vec2(0.3, 0.6), 0.08, time + 2.0);
    float bubble3 = bubble(uv, vec2(0.7, 0.4), 0.12, time + 4.0);

    col += vec3(bubble1 + bubble2 + bubble3) * 0.5;
    col = applyColorAdjustments(col);
    FragColor = vec4(col, 1.0);
}
