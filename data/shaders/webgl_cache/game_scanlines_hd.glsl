#version 300 es
precision highp float;
precision highp int;
// Clean modern scanlines for HD displays. No curvature, no color shift.
out vec4 color;
in vec2 TexCoord;
#define tc TexCoord
uniform sampler2D samp;
uniform float time_f;
uniform vec2 iResolution;

void main(void) {
    vec3 c = texture(samp, tc).rgb;
    float line = mod(gl_FragCoord.y, 2.0);
    float dim = line < 1.0 ? 1.0 : 0.85;
    color = vec4(c * dim, 1.0);
}
