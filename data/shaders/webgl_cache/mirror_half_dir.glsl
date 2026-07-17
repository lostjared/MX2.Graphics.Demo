#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
#define tc TexCoord
out vec4 color;
uniform sampler2D samp;
uniform float time_f;

void main(void)
{
    vec2 uv = tc;
    float duration = 2.0;
    float totalDuration = 2.0 * duration;
    float currentTime = mod(time_f, totalDuration);

    if (currentTime < duration) {
        if (uv.x > 0.5) {
            uv.x = 1.0 - uv.x;
        }
    } else {
        if (uv.x < 0.5) {
            uv.x = 1.0 - uv.x;
        }
    }
    
    color = texture(samp, uv);
}
