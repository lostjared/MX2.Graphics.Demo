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
    float foldAmount = sin(time_f * 3.14159);
    vec2 foldedTC = vec2(tc.x, abs(foldAmount * (tc.y - 0.5)) + 0.5 * sign(foldAmount));
    color = texture(samp, foldedTC);
}
