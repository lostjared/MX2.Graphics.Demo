#version 300 es
precision highp float;

in vec2 TexCoord;
out vec4 FragColor;
uniform float alpha_r;
uniform float alpha_g;
uniform float alpha_b;
uniform float current_index;
uniform float timeval;
uniform float alpha;
uniform vec3 vpos;
uniform vec4 optx_val;
uniform vec4 optx;
uniform vec4 random_value;
uniform vec4 random_var;
uniform float alpha_value;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform sampler2D textTexture;
uniform float value_alpha_r, value_alpha_g, value_alpha_b;
uniform float index_value;
uniform float time_f;

uniform vec4 inc_valuex;
uniform vec4 inc_value;

uniform float restore_black;
uniform float restore_black_value;

void main(void)
{
    if(restore_black_value == 1.0 && texture(textTexture, TexCoord) == vec4(0, 0, 0, 1))
        discard;
    FragColor = texture(textTexture, TexCoord);
    ivec4 source = ivec4(FragColor * 255.0);
    ivec4 inc = ivec4(inc_valuex);
    ivec4 inc_v = ivec4(inc_value);
    
    source += inc + inc_v;
    
    source[0] = source[0]%255;
    source[1] = source[1]%255;
    source[2] = source[2]%255;
    FragColor[0] = float(source[0])/255.0;
    FragColor[1] = float(source[1])/255.0;
    FragColor[2] = float(source[2])/255.0;
}



