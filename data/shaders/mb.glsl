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
    vec2 uv = applyZoomRotation(TexCoord, vec2(0.5)) * 2.0 - 1.0;
    uv *= iResolution.y / iResolution.x;

    vec2 center = vec2(0.0, 0.0);
    vec2 offset = uv - center;
    float dist = length(offset);
    float angle = atan(offset.y, offset.x);

    float pulse = sin(time * 2.0 + dist * 10.0 * iFrequency) * 0.15;
    float spiral = sin(angle * 5.0 + time * 2.0) * 0.15;
    float streTexCoordh = cos(time * 3.0 - dist * 15.0) * 0.2;

    vec2 spiralOffset = vec2(cos(angle), sin(angle)) * spiral;
    vec2 streTexCoordhOffset = normalize(offset) * (pulse + streTexCoordh);
    vec2 morphUV = uv + spiralOffset + streTexCoordhOffset;

    vec4 texColor = texture(textTexture, morphUV * 0.5 + 0.5);

    vec3 blended = vec3(0.0);
    float weight = 0.0;

    for (float x = -2.0; x <= 2.0; x++) {
        for (float y = -2.0; y <= 2.0; y++) {
            vec2 textTextureleUV = morphUV + vec2(x, y) * 0.005;
            vec4 textTextureleColor = texture(textTexture, textTextureleUV * 0.5 + 0.5);
            blended += textTextureleColor.rgb;
            weight += 1.0;
        }
    }

    blended /= weight;
    blended = mix(texColor.rgb, blended, 0.5);

    float shimmer = sin(dist * 20.0 - time * 5.0) * 0.1 * iAmplitude + 1.0;
    FragColor = vec4(blended * shimmer, texColor.a);
}

