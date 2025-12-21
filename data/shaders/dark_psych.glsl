#version 300 es
precision highp float;

in vec2 TexCoord;
out vec4 FragColor;
uniform float time_f;
uniform vec2 iResolution;
uniform sampler2D textTexture;
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
    vec2 uv = (TexCoord - 0.5) * 2.0;
    uv.x *= iResolution.x / iResolution.y;
    time = time * 0.5 * iAmplitude;
    float sine1 = sin(uv.x * 10.0 * iFrequency + time) * cos(uv.y * 10.0 + time);
    float sine2 = sin(uv.y * 15.0 - time) * cos(uv.x * 15.0 - time);
    float sine3 = sin(sqrt(uv.x * uv.x + uv.y * uv.y) * 20.0 + time);
    float pattern = sine1 + sine2 + sine3;
    float FragColorIntensity = pattern * 0.5 + 0.5;
    vec3 psychedelicColor = vec3(
        sin(FragColorIntensity * 6.28318 + 0.0) * 0.5 + 0.5,
        sin(FragColorIntensity * 6.28318 + 2.09439) * 0.5 + 0.5,
        sin(FragColorIntensity * 6.28318 + 4.18879) * 0.5 + 0.5
    );
    vec3 baseColor = psychedelicColor;
    uv = TexCoord * 2.0 - 1.0;
    float t = mod(time, 20.0);
    uv.y *= iResolution.y / iResolution.x;
    float angle = atan(uv.y, uv.x) + t * 20.0;
    vec3 rainbowColor = rainbow(angle / (2.0 * 3.14159));
    vec4 originalColor = texture(textTexture, TexCoord);
    vec3 blendedColor = mix(originalColor.rgb, rainbowColor, 0.5);
    FragColor = vec4(sin(blendedColor * t), originalColor.a);
    vec4 mixResult = mix(FragColor, vec4(baseColor, 1.0), 0.5);
    vec3 col = applyColorAdjustments(mixResult.rgb);
    FragColor = vec4(col, mixResult.a);
}
