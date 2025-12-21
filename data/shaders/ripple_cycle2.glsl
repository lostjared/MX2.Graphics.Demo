#version 300 es
precision highp float;

in vec2 TexCoord;
out vec4 FragColor;
uniform float time_f;
uniform sampler2D textTexture;
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
    vec2 normPos = gl_FragCoord.xy / iResolution.xy;
    
    vec2 mirroredPos = vec2(
        abs(fract(normPos.x * 2.0) - 0.5) * 2.0,
        abs(fract(normPos.y * 2.0) - 0.5) * 2.0
    );
    
    float time_t = pingPong(time, 6.0) + 2.0;
    float cycle = sin(time_t * 0.5 * iAmplitude);
    float movingPhase = mirroredPos.x + mirroredPos.y + cycle;
    float phase = sin(movingPhase * 10.0 * iFrequency - time * 2.0);
    
    vec2 TexCoordAdjusted = TexCoord + vec2(phase * 0.302, phase * 0.302);
    vec4 texCol = texture(textTexture, TexCoordAdjusted);
    vec3 finalCol = applyColorAdjustments(texCol.rgb);
    FragColor = vec4(finalCol, texCol.a);
}
