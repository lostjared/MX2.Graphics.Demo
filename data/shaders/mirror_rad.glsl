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

float pingPong(float t, float l) {
    return abs(mod(t, 2.0 * l) - l);
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
    float streTexCoordh = 1.0 + pingPong(time, 2.0) * 0.5 * iAmplitude;

    if (uv.x < 0.5) {
        uv.x = 0.5 - (0.5 - uv.x) * streTexCoordh;
        uv.x = 1.0 - uv.x;
    } else {
        uv.x = 0.5 + (uv.x - 0.5) * streTexCoordh;
    }

    float radius = 1.0;
    vec2 center = vec2(0.5, 0.5) * iResolution;
    vec2 texCoord = uv * iResolution;
    vec2 delta = texCoord - center;
    float dist = length(delta);
    float maxRadius = min(iResolution.x, iResolution.y) * radius;

    if (dist < maxRadius) {
        float scaleFactor = pingPong(time, 8.0) * (1.0 - pow(dist / maxRadius, 2.0));
        vec2 direction = normalize(delta);
        texCoord += direction * scaleFactor * 50.0;
    }

    uv = texCoord / iResolution;
    vec4 texCol = texture(textTexture, uv);
    vec3 finalCol = applyColorAdjustments(texCol.rgb);
    FragColor = vec4(finalCol, texCol.a);
}
