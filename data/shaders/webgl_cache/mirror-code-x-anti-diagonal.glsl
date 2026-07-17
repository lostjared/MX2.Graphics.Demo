#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
#define tc TexCoord
out vec4 color;

uniform sampler2D samp;

void main() {
    vec2 uv = tc;
    if (uv.x + uv.y > 1.0) {
        uv = 1.0 - uv.yx;
    }
    color = texture(samp, uv);
}
