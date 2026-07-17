#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
#define tc TexCoord
out vec4 color;
uniform sampler2D samp;
uniform float time_f;

void main(void) {
    float amplitude = sin(time_f * 5.0) * 2.0;
    float distFromCenter = abs(tc.y - 0.5);
    vec2 distorted_tc = tc;
    distorted_tc.y += amplitude * (0.5 - distFromCenter) * distFromCenter;
    distorted_tc = clamp(distorted_tc, vec2(0.0), vec2(1.0));
    color = texture(samp, distorted_tc);
}
