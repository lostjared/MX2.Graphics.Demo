#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
#define tc TexCoord
out vec4 color;
uniform sampler2D samp;
void main(void) {
    color = texture(samp, tc);
}

