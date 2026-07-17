#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
#define tc TexCoord
out vec4 color;

uniform sampler2D samp;

void main(void) {
    vec2 mirrored_tc = abs(fract(tc) - 0.5) + 0.5;
    vec4 color1 = texture(samp, mirrored_tc);
    vec4 color2 = texture(samp, mirrored_tc * 0.5);
    vec4 color3 = texture(samp, mirrored_tc * 0.25);
    vec4 color4 = texture(samp, mirrored_tc * 0.125);
    color = (color1 * 0.4) + (color2 * 0.3) + (color3 * 0.2) + (color4 * 0.1);
}

