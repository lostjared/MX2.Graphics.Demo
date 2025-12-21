#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D textTexture;
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

void main(void)
{
    float time = time_f * iSpeed;
    vec2 center = vec2(0.5, 0.5);
    float angle = time;
    vec2 TexCoord1 = TexCoord;
    vec2 TexCoord2 = TexCoord - center;
    TexCoord2 *= 0.8;
    TexCoord2 = vec2(
        TexCoord2.x * cos(angle + 1.0) - TexCoord2.y * sin(angle + 1.0),
        TexCoord2.x * sin(angle + 1.0) + TexCoord2.y * cos(angle + 1.0)
    );
    TexCoord2 += center;
    TexCoord2 = fract(TexCoord2);
    vec2 TexCoord3 = TexCoord - center;
    TexCoord3 *= 0.6;
    TexCoord3 = vec2(
        TexCoord3.x * cos(angle + 2.0) - TexCoord3.y * sin(angle + 2.0),
        TexCoord3.x * sin(angle + 2.0) + TexCoord3.y * cos(angle + 2.0)
    );
    TexCoord3 += center;
    TexCoord3 = fract(TexCoord3);
    vec2 TexCoord4 = TexCoord - center;
    TexCoord4 *= 0.4;
    TexCoord4 = vec2(
        TexCoord4.x * cos(angle + 3.0) - TexCoord4.y * sin(angle + 3.0),
        TexCoord4.x * sin(angle + 3.0) + TexCoord4.y * cos(angle + 3.0)
    );
    TexCoord4 += center;
    TexCoord4 = fract(TexCoord4);
    vec4 FragColor1 = texture(textTexture, TexCoord1);
    vec4 FragColor2 = texture(textTexture, TexCoord2);
    vec4 FragColor3 = texture(textTexture, TexCoord3);
    vec4 FragColor4 = texture(textTexture, TexCoord4);
    FragColor = (FragColor1 + FragColor2 + FragColor3 + FragColor4) * 0.25;
}

