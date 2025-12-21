#version 300 es
precision highp float;


layout (location=0) in vec3 pos;
layout (location=1) in vec2 texCoord;

out vec2 TexCoord;
out vec3 vpos;
out float alpha_r;
out float alpha_g;
out float alpha_b;
out float current_index;
out float timeval;
out float alpha;
out vec4 optx_val;
uniform vec4 optx;
out vec4 random_value;
uniform vec4 random_var;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform float value_alpha_r, value_alpha_g, value_alpha_b;
uniform float index_value;
uniform float time_f;
uniform float alpha_value;
uniform vec2 iResolution;
uniform float restore_black;
out float restore_black_value;
out vec2 iResolution_;
uniform sampler2D textTexture;
uniform sampler2D mat_textTexture;
uniform vec2 mat_size;
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

void main()
{
    float time = time_f * iSpeed;
    gl_Position = proj_matrix * mv_matrix * vec4(pos,1.0);
    TexCoord = texCoord;
    alpha_r = value_alpha_r;
    alpha_g = value_alpha_g;
    alpha_b = value_alpha_b;
    current_index = index_value;
    timeval = time;
    alpha = alpha_value;
    vpos = pos;
    optx_val = optx;
    random_value = random_var;
    restore_black_value = restore_black;
    iResolution_ = iResolution;
}
