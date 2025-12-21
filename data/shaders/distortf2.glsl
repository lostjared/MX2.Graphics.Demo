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
    vec2 center = vec2(0.5, 0.5);
    vec2 offset = uv - center;
    float dist = length(offset);

    float pulse = sin(dist * 30.0 - time * 10.0 * iFrequency) * 0.1 * iAmplitude;
    vec2 pulseUV = uv + pulse * normalize(offset);
    float fractalFactor = cos(length(offset) * 15.0 + time * 5.0) * 0.2;
    vec2 fractalUV = pulseUV + fractalFactor * normalize(offset);
    vec2 explosionUV = fractalUV + dist * vec2(cos(time * 4.0), sin(time * 4.0)) * 0.2;
    vec3 rainbow = vec3(0.5 + 0.5 * sin(time + explosionUV.x * 10.0),
                        0.5 + 0.5 * sin(time + explosionUV.y * 10.0),
                        0.5 + 0.5 * sin(time * 2.0));
    float angle = atan(offset.y, offset.x) + sin(time * 3.0) * 2.0;
    vec2 swirlUV = center + dist * vec2(cos(angle), sin(angle)) * 1.5;

    vec2 combinedUV = mix(swirlUV, explosionUV, 0.7);
    vec4 texColor = texture(textTexture, combinedUV);
    vec3 intenseColor = texColor.rgb * rainbow * 2.0;
    float glow = exp(-10.0 * dist) * 2.0;
    vec3 finalColor = mix(intenseColor, vec3(glow), 0.4);

    FragColor = vec4(finalColor, texColor.a);
}

