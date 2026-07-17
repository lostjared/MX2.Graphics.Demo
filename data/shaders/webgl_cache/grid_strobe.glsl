#version 300 es
precision highp float;
precision highp int;
in vec2 TexCoord;
#define tc TexCoord
out vec4 mxWebGLFragColor;
vec4 mxOutputColor;
uniform float alpha_r;
uniform float alpha_g;
uniform float alpha_b;
uniform float alpha;
uniform vec4 optx;
uniform vec4 random_var;
uniform float alpha_value;
uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform sampler2D samp;
uniform float value_alpha_r, value_alpha_g, value_alpha_b;
uniform float index_value;
uniform float time_f;

uniform float restore_black;

void mxShaderMain()
{
    mxOutputColor = texture(samp, tc);
    ivec3 source;
    for(int i = 0; i < 3; ++i) {
        source[i] = int(255.0 * mxOutputColor[i]);
    }
    vec2 cord1 = vec2(tc[0]/3.0, tc[1]/3.0);
    vec2 cord2 = vec2(tc[0]/6.0, tc[1]/6.0);
    vec2 cord3 = vec2(tc[0]/9.0, tc[1]/9.0);
    vec4 col1 = texture(samp, cord1);
    vec4 col2 = texture(samp, cord2);
    vec4 col3 = texture(samp, cord3);
    vec2 val = gl_FragCoord.xy / 3.0 * alpha_r;
    vec2 f = fract(val);
    mxOutputColor[0] = (mxOutputColor[0] + col1[2]) * f[0];
    mxOutputColor[1] = (mxOutputColor[1] + col2[1]) * f[1];
    mxOutputColor[2] = (mxOutputColor[2] + col3[0]) * f[0]+f[1];
    ivec3 int_color;
    for(int i = 0; i < 3; ++i) {
        int_color[i] = int(255.0 * mxOutputColor[i]);
        int_color[i] = int_color[i]^source[i];
        if(int_color[i] > 255)
            int_color[i] = int_color[i]%255;
        mxOutputColor[i] = float(int_color[i]) / 255.0;
    }
}


void main() {
    mxShaderMain();
    mxWebGLFragColor = mxOutputColor;
}
