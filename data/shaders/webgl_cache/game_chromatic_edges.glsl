#version 300 es
precision highp float;
precision highp int;
// Lens chromatic aberration that grows toward the edges of the frame.
out vec4 color;
in vec2 TexCoord;
#define tc TexCoord
uniform sampler2D samp;
uniform float time_f;
uniform vec2 iResolution;

void main(void) {
    vec2 v = tc - 0.5;
    float r2 = dot(v, v);
    float k = r2 * 0.04;
    vec2 dir = v;
    float cr = texture(samp, tc + dir * k * 1.0).r;
    float cg = texture(samp, tc                 ).g;
    float cb = texture(samp, tc - dir * k * 1.0).b;
    color = vec4(cr, cg, cb, 1.0);
}
