#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
#define tc TexCoord
out vec4 color;

uniform sampler2D samp;

void main() {
    vec2 p = tc - 0.5;
    if (abs(p.y) > abs(p.x)) {
        p = p.yx;
    }
    p.y = abs(p.y);
    vec2 uv = p + 0.5;
    color = texture(samp, uv);
}
