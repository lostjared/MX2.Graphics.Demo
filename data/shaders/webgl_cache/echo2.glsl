#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
#define tc TexCoord
out vec4 color;
uniform sampler2D samp;

void main(void) {
    color = texture(samp, tc);
    vec4 color2 = texture(samp, tc / 4.0);
    vec4 color3 = texture(samp, tc/ 8.0);
    vec4 color4 = texture(samp, tc/ 12.0);
    color = (color * 0.4) + (color2 * 0.4) + (color3 * 0.4) + (color4 * 0.4) ;
}