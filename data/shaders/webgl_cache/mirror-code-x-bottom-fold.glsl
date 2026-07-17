#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
#define tc TexCoord
out vec4 color;

uniform sampler2D samp;

void main() {
    vec2 uv = vec2(tc.x, 0.5 - abs(tc.y - 0.5));
    color = texture(samp, uv);
}
