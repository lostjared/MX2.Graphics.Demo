#version 300 es
precision highp float;
precision highp int;
in vec2 TexCoord;
#define tc TexCoord
out vec4 mxWebGLFragColor;
vec4 mxOutputColor;
uniform float alpha_value;
uniform sampler2D samp;
uniform float time_f;


void mxShaderMain()
{
    mxOutputColor = texture(samp, tc);
    ivec3 source;
    for(int i = 0; i < 3; ++i) {
        source[i] = int(255.0 * mxOutputColor[i]);
    }
    vec2 cord1 = vec2(tc[0]/2.0, tc[1]/2.0);
    vec2 cord2 = vec2(tc[0]/4.0, tc[1]/4.0);
    vec2 cord3 = vec2(tc[0]/8.0, tc[1]/8.0);
    vec4 col1 = texture(samp, cord1);
    vec4 col2 = texture(samp, cord2);
    vec4 col3 = texture(samp, cord3);
    mxOutputColor[0] = mxOutputColor[0] * col1[0];
    mxOutputColor[1] = mxOutputColor[1] * col2[1];
    mxOutputColor[2] = mxOutputColor[2] * col3[2];
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
