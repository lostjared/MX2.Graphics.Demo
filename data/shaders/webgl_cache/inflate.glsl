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
    float inflate = abs(sin(time_f * 10.0)) * 0.5 + 0.5;
    vec2 adjusted_tc = (tc - 0.5) * vec2(inflate, inflate) + 0.5;
    color = texture(samp, adjusted_tc);
}
