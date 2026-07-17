#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
#define tc TexCoord
out vec4 color;
uniform sampler2D samp;

void main(void) {
     color = texture(samp, tc);
    vec4 color2 = texture(samp, tc / 2.0);
    vec4 color3 = texture(samp, tc/ 4.0);
    vec4 color4 = texture(samp, tc/ 8.0);
    color[2] = (0.4 * color[2]) + (0.4 * color2[1]) + (0.4 * color3[1]) + (0.4 * color4[0]);
    color[1] = (0.4 * color[1]) + (0.4 * color2[1]) + (0.4 * color3[2]) + (0.4 * color4[0]);
    color[0] = (0.4 * color[0]) + (0.4 * color2[2]) + (0.4 * color3[2]) + (0.4 * color4[1]);
}