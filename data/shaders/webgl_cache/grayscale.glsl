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
    vec4 texColor = texture(samp, tc);
    float gray = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));
    color = vec4(vec3(gray), texColor.a);
}
