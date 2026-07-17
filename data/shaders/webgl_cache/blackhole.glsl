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
    vec2 uv = tc * 2.0 - 1.0;
    uv *= iResolution.x / iResolution.y;
    float dist = length(uv);
    float timeEffect = mod(time_f * 0.1, 3.0);
    float pullIntensity = pow(dist, 1.0 + 4.0 * timeEffect);
    vec2 pulledUV = uv * pullIntensity;
    pulledUV = (pulledUV / (iResolution.x / iResolution.y)) * 0.5 + 0.5;
    vec4 texColor = texture(samp, sin(pulledUV * timeEffect));
    color = vec4(texColor.rgb, 1.0);
}
