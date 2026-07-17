#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
#define tc TexCoord
out vec4 color;

uniform sampler2D samp;

float mirrorRepeat(float value) {
    return 1.0 - abs(mod(value, 2.0) - 1.0);
}

void main() {
    vec2 uv = vec2(mirrorRepeat(tc.x * 6.0), tc.y);
    color = texture(samp, uv);
}
