#shader vertex
#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

uniform mat4 u_VP;

uniform vec3 u_Voxel;

out vec2 v_Uv;

void main()
{
    v_Uv = uv;
    gl_Position = u_VP * vec4((position - 0.5) * 1.0075 + 0.5 + u_Voxel, 1.0);
}


#shader fragment
#version 430 core

layout(location = 0) out vec4 color;

#define minSize 0.045
#define maxSize 1.0 - minSize

uniform sampler2D u_Texture;

in vec2 v_Uv;

void main()
{
    if (v_Uv.x > minSize && v_Uv.x < maxSize && v_Uv.y > minSize && v_Uv.y < maxSize)
        discard;
    
    color = vec4(0.45, 0.45, 0.45, 0.65);
}
