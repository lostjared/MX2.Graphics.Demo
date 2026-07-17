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
    vec2 uv = tc;

    float scrambleAmountX = sin(time_f * 20.0) * 0.5 + 0.5;
    float scrambleAmountY = cos(time_f * 20.0) * 0.5 + 0.5;

    uv.x += sin(uv.y * 100.0 + time_f * 20.0) * scrambleAmountX * 0.1;
    uv.y += cos(uv.x * 100.0 + time_f * 20.0) * scrambleAmountY * 0.1;

    vec2 noise = vec2(
        sin(time_f + uv.y * 50.0) * 0.05,
        cos(time_f + uv.x * 50.0) * 0.05
    );

    uv += noise;

    color = texture(samp, uv);
}
