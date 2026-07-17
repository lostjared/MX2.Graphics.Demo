#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
#define tc TexCoord

uniform sampler2D samp;
uniform vec2 iResolution;
uniform float time_f;

void main() {
    float size = mod(time_f, 30.0);
    float pixelSize = 1.5 * size;
    vec2 pixelCount = iResolution / pixelSize;
    vec2 pixelatedTC = floor(tc * pixelCount) / pixelCount;
    color = texture(samp, pixelatedTC);
}
