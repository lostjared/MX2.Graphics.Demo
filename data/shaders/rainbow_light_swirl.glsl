#version 300 es
precision highp float;

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
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

vec3 rainbow(float t) {
    t = fract(t);
    float r = abs(t * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(t * 6.0 - 2.0);
    float b = 2.0 - abs(t * 6.0 - 4.0);
    return clamp(vec3(r, g, b), 0.0, 1.0);
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
    vec2 centeredCoord = (TexCoord * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
    float angle = atan(centeredCoord.y, centeredCoord.x) + time;
    float radius = length(centeredCoord);
    float spiral = sin(10.0 * angle - 3.0 * time) * exp(-3.0 * radius);
    vec3 lightColor = vec3(0.1, 0.5, 0.8) * 0.5 * (1.0 + spiral);
    lightColor = sin(lightColor * time);
    vec4 texColor = texture(textTexture, TexCoord);

    vec2 uv = applyZoomRotation(TexCoord, vec2(0.5)) * 2.0 - 1.0;
    uv.y *= iResolution.y / iResolution.x;
    float t = mod(time, 15.0);
    float wave = sin(uv.x * 10.0 * iFrequency + t * 2.0) * 0.1 * iAmplitude;
    float expand = 0.5 + 0.5 * sin(t * 2.0);
    vec2 spiral_uv = uv * expand + vec2(cos(t), sin(t)) * 0.2;
    float new_angle = atan(spiral_uv.y + wave, spiral_uv.x) + t * 2.0;
    vec3 rainbow_color = rainbow(new_angle / (2.0 * 3.14159));
    vec3 blended = mix(texColor.rgb * lightColor, rainbow_color, 0.5);
    vec3 finalCol = applyColorAdjustments(sin(blended * t));
    FragColor = vec4(finalCol, texColor.a);
}
