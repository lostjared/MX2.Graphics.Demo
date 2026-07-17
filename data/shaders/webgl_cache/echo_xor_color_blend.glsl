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

vec4 color_blend(vec4 mxOutputColor) {
    vec4 color2 = mxOutputColor;
    ivec4 color_source = ivec4(mxOutputColor * 255.0);
    mxOutputColor = mxOutputColor*alpha;
    ivec4 colori = ivec4(mxOutputColor * 255.0);
    for(int i = 0; i < 3; ++i) {
        if(colori[i] >= 255)
            colori[i] = colori[i]%255;
        
        if(color_source[i] >= 255)
            color_source[i] = color_source[i]%255;
        
        colori[i] = colori[i] ^ color_source[i];
        mxOutputColor[i] = float(colori[i]) / 255.0;
    }
    
    for(int i = 0; i < 3; ++i)
        if(mxOutputColor[i] < 0.2) mxOutputColor[i] = color2[i];
    return mxOutputColor;
}

void mxShaderMain()
{
    mxOutputColor = texture(samp, tc);
    vec4 color2 = texture(samp, tc / 2.0);
    vec4 color3 = texture(samp, tc/ 4.0);
    vec4 color4 = texture(samp, tc/ 8.0);
    mxOutputColor = (mxOutputColor * 0.4) + (color2 * 0.4) + (color3 * 0.4) + (color4 * 0.4) ;
    
    mxOutputColor = color_blend(mxOutputColor);
}


void main() {
    mxShaderMain();
    mxWebGLFragColor = mxOutputColor;
}
