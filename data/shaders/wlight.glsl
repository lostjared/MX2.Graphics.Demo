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

void main(void) {
    float time = time_f * iSpeed;
    float speed = 1.5;
    float amplitude = 0.02;
    float frequency = 6.0;
    float twist = 8.0;

    vec2 vortexCenter = vec2(-0.5, 1.5);
    vec2 adjustedTC = TexCoord - vortexCenter;
    float angle = atan(adjustedTC.y, adjustedTC.x);
    float radius = length(adjustedTC);
    float vortex = sin(twist * angle + radius * frequency - time * speed) * amplitude * radius;

    vec2 vortexDistortedTC = TexCoord + normalize(adjustedTC) * vortex;

    float whirlSpeed = 2.0;
    float whirlFrequency = 8.0;
    float whirlAmplitude = 0.03;
    float whirlScale = 0.5 + 0.5 * sin(time * whirlSpeed);
    vec2 whirlCenter = vec2(0.5, 0.5);
    vec2 whirlAdjustedTC = (TexCoord - whirlCenter) * whirlScale;
    float whirlAngle = atan(whirlAdjustedTC.y, whirlAdjustedTC.x);
    float whirlRadius = length(whirlAdjustedTC);
    float whirlDistortion = sin(whirlFrequency * whirlAngle + whirlRadius * whirlFrequency - time * whirlSpeed) * whirlAmplitude;

    vec2 whirlDistortedTC = TexCoord + normalize(whirlAdjustedTC) * whirlDistortion;

    vec4 originalColor = texture(textTexture, TexCoord);
    vec4 vortexColor = texture(textTexture, vortexDistortedTC);
    vec4 whirlColor = texture(textTexture, whirlDistortedTC);

    float lightEffect = sin(whirlRadius * 10.0 * iFrequency - time * 5.0) * 0.5 + 0.5;

    vec3 rainbow = vec3(
        sin(time + 0.0) * 0.5 + 0.5,
        sin(time + 2.094) * 0.5 + 0.5,
        sin(time + 4.188) * 0.5 + 0.5
    );
    vec4 rainbowLight = vec4(rainbow, 1.0) * lightEffect;

    vec4 mixResult = mix(originalColor, mix(vortexColor, whirlColor, 0.5), 0.6 + 0.4 * sin(time));
    vec3 col = applyColorAdjustments(mixResult.rgb);
    FragColor = vec4(col, mixResult.a);
    FragColor += rainbowLight * (1.0 - lightEffect) * 0.3;
}

