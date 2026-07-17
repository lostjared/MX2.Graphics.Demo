#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
#define tc TexCoord
out vec4 color;
uniform sampler2D samp;
uniform float time_f;

void main() {
    
    color = texture(samp, tc);
    vec2 uv = tc;
    uv.x -= 0.05;
    uv.y -= 0.05;
    vec4 color2 = texture(samp, uv);
    color = mix(color, color2, 0.5);
}
