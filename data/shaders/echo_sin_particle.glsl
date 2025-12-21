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

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec2 hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(dot(hash(i + vec2(0.0, 0.0)), f - vec2(0.0, 0.0)), dot(hash(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0)), u.x),
        mix(dot(hash(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0)), dot(hash(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0)), u.x),
        u.y);
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
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    vec3 particleCol = vec3(0.0);

    float numParticles = 50.0;
    for (float i = 0.0; i < numParticles; i += 1.0) {
        float t = time * 0.1 * iAmplitude + i * 6.28 / numParticles;
        vec2 pos = vec2(sin(t + noise(vec2(i, t)) * 10.0 * iFrequency), cos(t + noise(vec2(i, t)) * 10.0)) * 0.3 + 0.5;
        float d = length(uv - pos);
        particleCol += vec3(0.1, 0.5, 0.8) * (1.0 - d) * exp(-d * 10.0);
    }

    vec4 FragColor1 = vec4(particleCol, 1.0);
    vec4 mixResult = mix(texture(textTexture, TexCoord), FragColor1, 0.5);

    vec4 FragColor2 = texture(textTexture, TexCoord / 2.0);
    vec4 FragColor3 = texture(textTexture, TexCoord / 4.0);
    vec4 FragColor4 = texture(textTexture, TexCoord / 8.0);
    vec4 echoResult = (mixResult * 0.4) + (FragColor2 * 0.4) + (FragColor3 * 0.4) + (FragColor4 * 0.4);

    float time_t = pingPong(time, 10.0) + 2.0;
    vec3 final_col = sin(echoResult.rgb * time_t);
    vec3 col = applyColorAdjustments(final_col);
    FragColor = vec4(col, echoResult.a);
}
