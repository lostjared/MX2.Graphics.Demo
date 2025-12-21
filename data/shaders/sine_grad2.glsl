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

vec3 rainbow(float t) {
    float r = sin(6.28318 * t + 0.0);
    float g = sin(6.28318 * t + 2.09439);
    float b = sin(6.28318 * t + 4.18878);
    return vec3(r, g, b) * 0.5 + 0.5;
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
    float swirl_strength = 0.1;
    float swirl_speed = 1.0;
    float angle = swirl_speed * time + length(TexCoord - 0.5);
    vec2 swirl_offset = vec2(cos(angle), sin(angle)) * swirl_strength;
    vec2 swirl_TexCoord = TexCoord + swirl_offset;
    swirl_TexCoord = clamp(swirl_TexCoord, vec2(0.0), vec2(1.0));
    float scale = 5.0;
    float speed = 0.2;
    float gradient_pos = 0.5 + 0.5 * sin((swirl_TexCoord.x * scale + swirl_TexCoord.y * scale) + time * speed);
    vec3 FragColor_gradient = rainbow(gradient_pos);
    vec4 ctx = texture(textTexture, swirl_TexCoord);
    FragColor = vec4(sin(ctx.rgb * time), ctx.a);
    FragColor = mix(FragColor, vec4(FragColor_gradient, 1.0), 0.5);
}
