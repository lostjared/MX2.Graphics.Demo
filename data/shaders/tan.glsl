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

void main(void)
{
    if(restore_black_value == 1.0 && texture(textTexture, TexCoord) == vec4(0, 0, 0, 1))
        discard;
    FragColor = texture(textTexture, TexCoord);
    FragColor[0] += tan(FragColor[0] * timeval);
    FragColor[1] += tan(FragColor[1] * timeval);
    FragColor[2] += tan(FragColor[2] * timeval);
    ivec3 int_FragColor;
    int_FragColor[0] = int(255.0 * FragColor[0]);
    int_FragColor[1] = int(255.0 * FragColor[1]);
    int_FragColor[2] = int(255.0 * FragColor[2]);
    if(int_FragColor[0] > 255)
        int_FragColor[0] = int_FragColor[0] % 255;
    if(int_FragColor[1] > 255)
        int_FragColor[1] = int_FragColor[1] % 255;
    if(int_FragColor[2] > 255)
        int_FragColor[2] = int_FragColor[2] % 255;
    FragColor[0] = float(int_FragColor[0]) / 255.0;
    FragColor[1] = float(int_FragColor[1]) / 255.0;
    FragColor[2] = float(int_FragColor[2]) / 255.0;
}

