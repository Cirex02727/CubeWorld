#shader vertex
#version 430 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

out vec2 v_Uv;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    v_Uv = uv;
}


#shader fragment
#version 430 core

out vec4 color;

uniform sampler2D u_Texture;

in vec2 v_Uv;

void main()
{
    color += texture(u_Texture, v_Uv);
}
