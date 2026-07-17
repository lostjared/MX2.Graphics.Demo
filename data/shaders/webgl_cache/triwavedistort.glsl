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
    float xDistort = cos(tc.y * 10.0 + time_f) * 0.1;
    float yDistort = sin(tc.x * 10.0 + time_f) * 0.1;
    float tanDistortX = tan(tc.x * 5.0 + time_f) * 0.05;
    float tanDistortY = tan(tc.y * 5.0 + time_f) * 0.05;
    vec2 distortedTC = tc + vec2(xDistort + tanDistortX, yDistort + tanDistortY);
    distortedTC = fract(distortedTC);
    color = texture(samp, distortedTC);
}
