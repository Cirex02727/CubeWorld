#shader vertex
#version 430 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

uniform mat4 u_VP;

out vec2 v_Uv;

void main()
{
    v_Uv = uv;
    gl_Position = u_VP * vec4(position, 0.0, 1.0);
}


#shader fragment
#version 430 core

layout(location = 0) out vec4 color;

uniform sampler2D u_Texture;

in vec2 v_Uv;

void main()
{
    color = texture(u_Texture, v_Uv);
    if (color.a == 0)
        discard;
}
