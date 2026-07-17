#version 300 es
precision highp float;
precision highp int;
in vec2 TexCoord;
#define tc TexCoord
out vec4 color;

uniform sampler2D samp;
uniform float time_f;
uniform vec2 iResolution;
uniform float restore_black;
uniform vec4 inc_valuex;

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void)
{
    color = texture(samp, tc);
    vec2 pos = gl_FragCoord.xy / iResolution;
    vec4 s = color * sin(inc_valuex / 255.0 * time_f);
    color[0] += s[0] * pos[0];
    color[1] += s[1] * pos[1];
 
    float time_t = pingPong(time_f, 20.0) + 2.0;
    
    color = sin(color * time_t);
    color.a = 1.0;
}
