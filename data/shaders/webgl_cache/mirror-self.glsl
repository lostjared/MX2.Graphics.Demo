#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
#define tc TexCoord

uniform sampler2D samp;

void main() {
    vec2 uv = 1.0 - abs(1.0 - 2.0 * tc);
    color = texture(samp, uv);
}
