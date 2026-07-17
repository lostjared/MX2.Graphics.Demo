#version 300 es
precision highp float;
precision highp int;
out vec4 color;
in vec2 TexCoord;
#define tc TexCoord
uniform sampler2D samp;
uniform vec2 iResolution;
uniform float time_f;
void main(void) {
    vec2 normCoord = tc;
    float warpAmount = cos(time_f);
    vec2 warp = vec2(
        sin(normCoord.y * 10.0 + time_f) * warpAmount,
        cos(normCoord.x * 10.0 + time_f) * warpAmount
    );
    vec2 warpedCoord = normCoord + warp;
    color = texture(samp, warpedCoord);
}
