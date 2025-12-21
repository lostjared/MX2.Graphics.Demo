#version 300 es
precision highp float;

in vec2 TexCoord;
out vec4 FragColor;
uniform float alpha_r;
uniform float alpha_g;
uniform float alpha_b;
uniform float current_index;
uniform float timeval;
uniform float alpha;
uniform vec3 vpos;
uniform vec4 optx_val;
uniform vec4 optx;
uniform vec4 random_value;
uniform vec4 random_var;
uniform float alpha_value;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform sampler2D textTexture;
uniform float value_alpha_r, value_alpha_g, value_alpha_b;
uniform float index_value;
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

void main(void)
{
    float time = time_f * iSpeed;
    float time_x = pingPong(time, 2.0) + 1.0;
    float time_y = pingPong(time, 4.0) + 1.0;
    float time_z = pingPong(time, 8.0) + 1.0;
    vec4 texCol = texture(textTexture, TexCoord);
    vec3 finalCol = applyColorAdjustments(texCol.rgb);
    FragColor = vec4(finalCol, texCol.a);
    vec4 FragColor2 = texture(textTexture, TexCoord /time_x);
    vec4 FragColor3 = texture(textTexture, TexCoord/time_y);
    vec4 FragColor4 = texture(textTexture, TexCoord/time_z);
    FragColor = (FragColor * 0.4) + (FragColor2 * 0.4) + (FragColor3 * 0.4) + (FragColor4 * 0.4) ;
    float time_q = pingPong(time, 25.0);
    FragColor = cos(sin(length(time_q) * FragColor));
}



