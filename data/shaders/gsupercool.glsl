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
    float frequency = 30.0;
    float amplitude = 0.05;
    float speed = 5.0;
    float t = time * speed;
    float noiseX = rand(uv * frequency + t);
    float noiseY = rand(uv * frequency - t);
    float distortionX = amplitude * pingPong(t + uv.y * frequency + noiseX, 1.0);
    float distortionY = amplitude * pingPong(t + uv.x * frequency + noiseY, 1.0);
    vec2 uv_distorted = uv + vec2(distortionX, distortionY);
    vec2 uv_kaleidoscope = uv_distorted * 2.0 - 1.0;
    uv_kaleidoscope.x *= iResolution.x / iResolution.y;
    float angle = atan(uv_kaleidoscope.y, uv_kaleidoscope.x);
    float radius = length(uv_kaleidoscope) * 1.4142;
    float segments = 8.0;
    angle = mod(angle, 6.28318 / segments);
    angle = abs(angle - 3.14159 / segments);
    uv_kaleidoscope = vec2(cos(angle), sin(angle)) * radius;
    uv_kaleidoscope = uv_kaleidoscope * 0.5 + 0.5;
    uv_kaleidoscope = clamp(uv_kaleidoscope, 0.0, 1.0);

    float time_t = pingPong(time * 0.5 * iAmplitude, 7.0) + 2.0;
    float pattern = cos(radius * 10.0 * iFrequency - time_t * 5.0);
    vec3 FragColorShift = vec3(
        0.5 + 0.5 * cos(pattern + time_t + 0.0),
        0.5 + 0.5 * cos(pattern + time_t + 2.094),
        0.5 + 0.5 * cos(pattern + time_t + 4.188)
    );

    vec4 texColorKaleido = texture(textTexture, uv_kaleidoscope);
    vec4 texColorDistorted = texture(textTexture, uv_distorted);

    vec3 finalColor = texColorKaleido.rgb * FragColorShift;
    vec4 blendedColor = mix(vec4(finalColor, texColorKaleido.a), texColorDistorted, 0.5);

    FragColor = sin(blendedColor * time_t);
}
