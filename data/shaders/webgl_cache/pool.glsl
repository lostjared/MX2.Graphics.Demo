#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
#define tc TexCoord

uniform sampler2D samp;
uniform float time_f;

void main(void) {
    float time_t = mod(time_f, 100.0);
    float scale = time_t;
    float speed = 16.0;
    float offset = sin(time_f * speed + tc.x * scale) * 0.05;
    vec2 tcOffset = vec2(tc.x, tc.y + offset);
   color = texture(samp, tcOffset);
}
