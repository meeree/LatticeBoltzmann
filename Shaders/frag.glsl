#version 450 core

out vec4 color;
uniform sampler2D tex;
in vec2 stream_tex_coord;

void main(void)
{
    float t = texture(tex, stream_tex_coord).r;
    color = mix(mix(vec4(1,0,0,1), vec4(1,1,1,1), t*0.1), vec4(0,1,0,1), (1/0.9)*(t - 0.1));
//    color = t > 0.8 ? vec4(0,0,0,1) : color;
//    color = t > 0.95 ? vec4(1,1,1,1) : color;
}
