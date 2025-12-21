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
    float hue = mod(t, 1.0) * 6.0;
    float c = 1.0;
    float x = 1.0 - abs(mod(hue, 2.0) - 1.0);
    vec3 rgb = hue < 1.0 ? vec3(c, x, 0.0) :
               hue < 2.0 ? vec3(x, c, 0.0) :
               hue < 3.0 ? vec3(0.0, c, x) :
               hue < 4.0 ? vec3(0.0, x, c) :
               hue < 5.0 ? vec3(x, 0.0, c) :
                           vec3(c, 0.0, x);
    return rgb;
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
    vec2 center = vec2(0.5, 0.5);
    vec2 dir = TexCoord - center;
    float dist = length(dir);
    float angle = atan(dir.y, dir.x);
    float radius = dist + sin(time * 2.0 + dist * 10.0 * iFrequency) * 0.05 * iAmplitude;
    vec2 spiral_offset = vec2(cos(angle), sin(angle)) * radius;
    vec2 spiral_TexCoord = center + spiral_offset;
    spiral_TexCoord = clamp(spiral_TexCoord, vec2(0.0), vec2(1.0));
    float gradient_pos = mod(dist * 3.0 - time * 0.5, 1.0);
    vec3 FragColor_gradient = rainbow(gradient_pos);
    vec4 ctx = texture(textTexture, TexCoord);
    float time_t = pingPong(time, 10.0) + 2.0;
    vec4 mixResult = mix(sin(ctx * time_t), vec4(FragColor_gradient, 1.0), 0.5);
    vec3 col = applyColorAdjustments(mixResult.rgb);
    FragColor = vec4(col, mixResult.a);
}

