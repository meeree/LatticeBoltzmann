#version 450 core

layout (location=0) in vec2 tex_coord;
out vec2 stream_tex_coord;
void main(void)
{
    vec2 position = (2.0*tex_coord - vec2(1.0, 1.0));
    gl_Position = vec4(position, 0.0, 1.0);
    stream_tex_coord = tex_coord;
}
