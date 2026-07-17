#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
#define tc TexCoord

uniform sampler2D samp;
uniform float time_f;
uniform vec2 iResolution;

void main(void) {
    vec2 uv = tc * 2.0 - 1.0;
    float angle = sin(time_f) * 3.14159265;
    float s = sin(angle);
    float c = cos(angle);
    uv = mat2(c, -s, s, c) * uv;
    uv = abs(uv);
    uv = uv * 0.5 + 0.5;
    color = texture(samp, uv);
}
