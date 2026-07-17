#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
#define tc TexCoord
out vec4 color;
uniform sampler2D samp;
uniform float time_f;

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}


void main(void) {
    float time_t =   pingPong(time_f, 10.0) + 2.0;
    vec2 uv = tc - 0.5;
    float len = length(uv);
    float factor = sqrt(len) * 0.5;
    uv *= (1.0 + sin(factor * time_t));
    color = texture(samp, uv + 0.5);
}
