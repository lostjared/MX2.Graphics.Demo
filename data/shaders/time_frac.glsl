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
uniform float restore_black;
uniform float restore_black_value;
uniform vec2 iResolution_;
uniform vec4 inc_value;


void main(void)
{
    if(restore_black_value == 1.0 && texture(textTexture, TexCoord) == vec4(0, 0, 0, 1))
        discard;
    FragColor = texture(textTexture, TexCoord);
    FragColor = (0.5 * FragColor) + (0.5 *  fract(inc_value/255.0 * time_f));
}


