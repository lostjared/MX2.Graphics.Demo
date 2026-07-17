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
    vec4 source = mxOutputColor;
    float val = cos((mxOutputColor[0]+mxOutputColor[1]+mxOutputColor[2])*alpha);
    
    for(int q = 0; q < 3; ++q)
    mxOutputColor[q] = mxOutputColor[q] * (val * mxOutputColor[q]/0.3);
    
    mxOutputColor = (0.5 * mxOutputColor) + (0.7 * source);
}


void main() {
    mxShaderMain();
    mxWebGLFragColor = mxOutputColor;
}
