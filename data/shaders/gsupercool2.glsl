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

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
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

    float angle = atan(uv.y - 0.5, uv.x - 0.5);
    float modulatedTime = pingPong(time, 5.0);
    angle += modulatedTime;

    vec2 rotatedTC;
    rotatedTC.x = cos(angle) * (uv.x - 0.5) - sin(angle) * (uv.y - 0.5) + 0.5;
    rotatedTC.y = sin(angle) * (uv.x - 0.5) + cos(angle) * (uv.y - 0.5) + 0.5;

    vec2 flow;
    flow.x = sin((rotatedTC.x + time) * 10.0 * iFrequency) * 0.02 * iAmplitude;
    flow.y = cos((rotatedTC.y + time) * 10.0) * 0.02;
    vec2 uv_flow = rotatedTC + flow;

    float frequency = 30.0;
    float amplitude = 0.05;
    float speed = 5.0;
    float t = time * speed;
    float noiseX = rand(uv_flow * frequency + t);
    float noiseY = rand(uv_flow * frequency - t);
    float distortionX = amplitude * pingPong(t + uv_flow.y * frequency + noiseX, 1.0);
    float distortionY = amplitude * pingPong(t + uv_flow.x * frequency + noiseY, 1.0);
    vec2 uv_distorted = uv_flow + vec2(distortionX, distortionY);

    float zoom = sin(time * 0.5) * 0.2 + 1.0;
    vec2 uv_zoom = (uv_distorted - vec2(0.5)) * zoom + vec2(0.5);

    vec4 FragColor_textTexturele = texture(textTexture, uv_zoom);

    vec3 FragColor_effect;
    FragColor_effect.r = sin(FragColor_textTexturele.r * 10.0 + time);
    FragColor_effect.g = sin(FragColor_textTexturele.g * 10.0 + time + 2.0);
    FragColor_effect.b = sin(FragColor_textTexturele.b * 10.0 + time + 4.0);

    FragColor = vec4(FragColor_effect, FragColor_textTexturele.a);
}
