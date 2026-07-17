#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
#define tc TexCoord

uniform sampler2D samp;
uniform vec2 iResolution;

void main() {
    vec2 uv = tc;
    if (uv.x < 0.5) {
        uv.x = 1.0 - uv.x;
    }
    color = texture(samp, uv);
}
