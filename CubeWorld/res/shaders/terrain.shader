#shader vertex
#version 430 core

#define MAX_BLOCKS_IDS 4096

layout(location = 0) in uint data;
layout(location = 1) in uint data1;

uniform mat4 u_VP;

uniform vec3 u_ChunkOff;

uniform bool u_DebugNormal;
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

out vec3 v_Color;
out vec3 v_Normal;
out vec3 v_FragPos;

flat out int v_Debug;

void main()
{
    // Extract Data
    float x = float( data        & 0x3Fu);
    float y = float((data >> 6 ) & 0x3Fu);
    float z = float((data >> 12) & 0x3Fu);

    uint w = (data >> 18) & 0x1F + 1u;
    uint h = (data >> 23) & 0x1F + 1u;

    vec2 uv = uvs[(data >> 28) & 0x3u];

    float ao = aos[data >> 30];

    uint id = data1 & 0x7FFu;

    vec3 norm = normals[(data1 >> 12) & 0x7u];
    
    vec3 pos = vec3(x, y, z) + u_ChunkOff;

    // Calculate Fragment Color
    if(u_DebugNormal)
    {
        v_Color = vec3((norm + 1) * 0.5);
        v_Debug = 1;
    }
    else if(u_DebugUV)
    {
        v_Color = vec3(uv, 0.0);
        v_Debug = 1;
    }
    else
    {
        v_Color = vec3(0.75, 0.35, 0.15) * ao;
        v_Debug = 0;
    }

    v_Normal = norm;
    v_FragPos = pos;

    gl_Position = u_VP * vec4(pos, 1.0);
}


#shader fragment
#version 430 core

layout(location = 0) out vec4 color;

uniform sampler2D u_Texture;

uniform vec3 u_CamPos;

in vec3 v_Color;
in vec3 v_Normal;
in vec3 v_FragPos;

flat in int v_Debug;

void main()
{
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    float ambientStrength = 0.1;

    vec3 ambient = ambientStrength * lightColor;

    vec3 lightDir = normalize(u_CamPos - v_FragPos);

    float diff = max(dot(v_Normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    if(v_Debug == 1 || true)
        color = vec4(v_Color, 1.0);
    else
        color = vec4((ambient + diffuse) * v_Color, 1.0);
}
