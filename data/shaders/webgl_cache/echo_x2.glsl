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
uniform vec4 inc_value;

uniform float restore_black;

void mxShaderMain()
{
    
    mxOutputColor = texture(samp, tc);
    ivec3 source;
    for(int i = 0; i < 3; ++i) {
        source[i] = int(255.0 * mxOutputColor[i]);
    }
    mxOutputColor = mxOutputColor * inc_value/255.0 * alpha;
    ivec3 int_color;
     for(int i = 0; i < 3; ++i) {
         int_color[i] = int(255.0 * mxOutputColor[i]);
         int_color[i] = int_color[i]^source[i];
         if(int_color[i] > 255)
             int_color[i] = int_color[i]%255;
         mxOutputColor[i] = float(int_color[i]) / 255.0;
     }
    
    vec4 color2 = texture(samp, tc / 2.0);
    vec4 color3 = texture(samp, tc/ 4.0);
    vec4 color4 = texture(samp, tc/ 8.0);
    mxOutputColor = (mxOutputColor * 0.4) + (color2 * 0.4) + (color3 * 0.4) + (color4 * 0.4) ;
}


void main() {
    mxShaderMain();
    mxWebGLFragColor = mxOutputColor;
}
