#shader vertex
#version 430 core

#define MAX_BLOCKS_IDS 4096

layout(location = 0) in uint data;
layout(location = 1) in uint data1;

struct Block
{
    uvec2 offset; // 1 uint offset + 1 padding
    vec2 uv;
};

layout(std140, binding = 0) uniform BlockUniform
{
    Block data[MAX_BLOCKS_IDS];
} blocks;

uniform mat4 u_VP;

uniform vec3 u_ChunkOff;

uniform bool u_DebugUV;

const vec2 uvs[4] = vec2[]
(
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

const float aos[4] = float[]
(
    0.15, 0.45, 0.7, 1.0
);

const vec3 normals[6] = vec3[]
(
    vec3( 0.0,  0.0,  1.0), // 0
    vec3( 0.0,  0.0, -1.0), // 1
    vec3( 0.0,  1.0,  0.0), // 2
    vec3( 0.0, -1.0,  0.0), // 3
    vec3( 1.0,  0.0,  0.0), // 4
    vec3(-1.0,  0.0,  0.0)  // 5
);

out vec2 v_UV;
out vec2 v_UVOff;
out float v_AO;

out vec3 v_Normal;
out vec3 v_FragPos;

void main()
{
    // Extract Data
    float x = float( data        & 0x3Fu);
    float y = float((data >> 6 ) & 0x3Fu);
    float z = float((data >> 12) & 0x3Fu);

    float w = float((data >> 18) & 0x1F) + 1.0;
    float h = float((data >> 23) & 0x1F) + 1.0;

    vec2 uv = uvs[(data >> 28) & 0x3u] * vec2(w, h);

    float ao = aos[data >> 30];

    uint id = data1 & 0x7FFu;

    vec3 norm = normals[(data1 >> 12) & 0x7u];

    vec3 pos = vec3(x, y, z) + u_ChunkOff;

    // Calculate Fragment Color
    v_UV = uv;
    v_UVOff = blocks.data[blocks.data[id].offset.x].uv;
    v_AO = ao;

    v_Normal = norm;
    v_FragPos = pos;


    gl_Position = u_VP * vec4(pos, 1.0);
}


#shader fragment
#version 430 core

layout(location = 0) out vec4 color;

uniform sampler2D u_Texture;

uniform vec3 u_CamPos;

uniform bool u_DebugNormal;
uniform bool u_DebugUV;

uniform vec2 u_Step;

in vec2 v_UV;
in vec2 v_UVOff;
in float v_AO;

in vec3 v_Normal;
in vec3 v_FragPos;

void main()
{
    if(u_DebugNormal)
    {
        color = vec4((v_Normal + 1.0) * 0.5, 1.0);
        return;
    }
    if(u_DebugUV)
    {
        color = vec4(v_UV, 0.0, 1.0);
        return;
    }

    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    float ambientStrength = 0.1;

    vec3 ambient = ambientStrength * lightColor;

    vec3 lightDir = normalize(u_CamPos - v_FragPos);

    float diff = max(dot(v_Normal, lightDir), 0.75);
    vec3 diffuse = diff * lightColor;


    color = texture(u_Texture, v_UVOff + fract(v_UV) * u_Step);
    if(color.a == 0)
        discard;

    if(color.a == 1)
        color *= vec4((ambient + diffuse) * v_AO, 1.0);
}
