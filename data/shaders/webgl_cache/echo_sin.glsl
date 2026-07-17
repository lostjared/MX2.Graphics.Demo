#version 300 es
precision highp float;
precision highp int;
in vec2 TexCoord;
#define tc TexCoord
out vec4 color;
uniform sampler2D samp;
uniform float time_f;

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void)
{
    color = texture(samp, tc);
    vec4 color2 = texture(samp, tc / 2.0);
    vec4 color3 = texture(samp, tc / 4.0);
    vec4 color4 = texture(samp, tc / 8.0);
    color = (color * 0.4) + (color2 * 0.4) + (color3 * 0.4) + (color4 * 0.4);
    
    float time_t = pingPong(time_f, 10.0) + 2.0;
    color = sin(color * time_t);
    color.a = 1.0;
}
