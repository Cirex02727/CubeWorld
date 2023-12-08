#include "Chunk.h"

#include "Core.h"
#include "CubeWorld.h"

#include "utils/Timer.h"
#include "utils/Instrumentor.h"

Chunk::~Chunk()
{
	GLCall(glDeleteVertexArrays(1, &m_VAO));
	GLCall(glDeleteBuffers(2, m_VBIO));

	delete[] m_Data;
}

void Chunk::Fill(SimplexNoise* noise)
{
    m_Stage = Stage::Filling;

    m_Data = new uint32_t[CHUNK_SIZEQ]{};

	// Fill Density
    uint16_t* data = new uint16_t[CHUNK_SIZES];
    if (data == nullptr)
        return;

    float scale = 0.00065f;

    for (uint16_t z = 0; z < CHUNK_SIZE; ++z)
        for (uint16_t x = 0; x < CHUNK_SIZE; ++x)
            data[x + z * CHUNK_SIZE] = (uint16_t)((noise->fractal(5, (x + m_Coord.x) * scale, (z + m_Coord.z) * scale) + 1) * 0.5f * CHUNK_MAX_MOUNTAIN);

    srand(unsigned int(time(NULL)));

    for (uint16_t z = 0; z < CHUNK_SIZE; ++z)
        for (uint16_t x = 0; x < CHUNK_SIZE; ++x)
        {
            uint16_t maxH = data[x + z * CHUNK_SIZE];

            for (uint16_t y = 0; y < CHUNK_SIZE; y++)
            {
                float newY = (float)y + m_Coord.y;
                if (newY >= maxH)
                    break;

                m_Data[x + y * CHUNK_SIZE + z * CHUNK_SIZES] = 1;
            }
        }

    delete[] data;

    m_Stage = Stage::Filled;
}

void Chunk::GenerateMesh(CubeWorld* world, Mesh& mesh)
{
    m_Stage = Stage::Building;

    // ZXY
    Chunk* chunks[27];
    // -Z-X-Y | -Z-X | -Z-X+Y | -Z-Y | -Z | -Z+Y | -Z+X-Y | -Z+X | -Z+X+Y
    //   -X-Y |   -X |   -X+Y |   -Y |        +Y |   +X-Y |   +X |   +X+Y
    // +Z-X-Y | +Z-X | +Z-X+Y | +Z-Y | +Z | +Z+Y | +Z+X-Y | +Z+X | +Z+X+Y
    if (!world->GetChunkNeighbors(this, m_Coord, chunks))
    {
        m_Stage = Stage::WaitingNeighbors;
        return;
    }

    int i, j, k, l, w, h, u, v, n = 0;
    FaceSide side;

    int x []{ 0, 0, 0 };
    int q []{ 0, 0, 0 };
    int du[]{ 0, 0, 0 };
    int dv[]{ 0, 0, 0 };
    int dt[]{ 0, 0, 0 };

    uint32_t voxelFace, voxelFace1;

    FaceMask mask[CHUNK_SIZES];
    memset(mask, 0, CHUNK_SIZES * sizeof(FaceMask));

    uint32_t indices = 0;

    for (bool backFace = true, b = false; b != backFace; backFace = backFace && b, b = !b)
        for (int d = 0; d < 3; ++d)
        {
            u = (d + 1) % 3;
            v = (d + 2) % 3;

            x[0] = 0;
            x[1] = 0;
            x[2] = 0;

            q[0] = 0;
            q[1] = 0;
            q[2] = 0;
            q[d] = 1;

            switch (d)
            {
            case 0: side = backFace ? FaceSide::Left   : FaceSide::Right; break;
            case 1: side = backFace ? FaceSide::Bottom : FaceSide::Top;   break;
            case 2: side = backFace ? FaceSide::Back   : FaceSide::Front; break;
            }

            for (x[d] = -1; x[d] < CHUNK_SIZE;)
            {
                n = 0;

                du[0] = 0;
                du[1] = 0;
                du[2] = 0;
                du[u] = 1;

                dv[0] = 0;
                dv[1] = 0;
                dv[2] = 0;
                dv[v] = 1;

                for (x[v] = 0; x[v] < CHUNK_SIZE; ++x[v])
                    for (x[u] = 0; x[u] < CHUNK_SIZE; ++x[u])
                    {
                        voxelFace  = (x[d] >= 0)             ? m_Data[x[0] + x[1] * CHUNK_SIZE + x[2] * CHUNK_SIZES]
                            : GetNeighborBlock(chunks, x, d, false, CHUNK_SIZE - 1);
                        voxelFace1 = (x[d] < CHUNK_SIZE - 1) ? m_Data[(x[0] + q[0]) + (x[1] + q[1]) * CHUNK_SIZE + (x[2] + q[2]) * CHUNK_SIZES]
                            : GetNeighborBlock(chunks, x, d, true,  0);

                        if (voxelFace == voxelFace1)
                        {
                            mask[n].ao.data = 0x03030303;
                            mask[n++].id    = 0;
                            continue;
                        }

                        if (backFace)
                        {
                            dt[0] = x[0];
                            dt[1] = x[1];
                            dt[2] = x[2];
                        }
                        else
                        {
                            dt[0] = x[0] + q[0];
                            dt[1] = x[1] + q[1];
                            dt[2] = x[2] + q[2];
                        }

                        CalculateAO(chunks, mask[n].ao, dt[0], dt[1], dt[2], du, dv);

                        // mask[n].ao.data = 0x03030303;
                        mask[n++].id = backFace ? voxelFace1 : voxelFace;
                    }

                ++x[d];

                n = 0;
                for (j = 0; j < CHUNK_SIZE; ++j)
                    for (i = 0; i < CHUNK_SIZE;)
                    {
                        if (!mask[n].id)
                        {
                            ++i;
                            ++n;
                            continue;
                        }

                        for (w = 1; i + w < CHUNK_SIZE && mask[n + w].id && mask[n + w] == mask[n]; ++w);

                        bool done = false;

                        for (h = 1; j + h < CHUNK_SIZE; ++h)
                        {
                            for (k = 0; k < w; ++k)
                            {
                                int indx = n + k + h * CHUNK_SIZE;
                                if (!mask[indx].id || mask[indx] != mask[n]) { done = true; break; }
                            }

                            if (done) break;
                        }

                        //if (!mask[n]->transparent)
                        if (true)
                        {
                            x[u] = i;
                            x[v] = j;

                            du[u] = w;
                            dv[v] = h;

                            const AO& ao = mask[n].ao;

                            int mW = w - 1, mH = h - 1;

                            mesh.vertices.push_back(VBO(x[0],                 x[1],                 x[2],                 mW, mH, 0, ao.vertices.v0));
                            mesh.vertices.push_back(VBO1(0, side));
                            mesh.vertices.push_back(VBO(x[0] + dv[0],         x[1] + dv[1],         x[2] + dv[2],         mW, mH, 1, ao.vertices.v1));
                            mesh.vertices.push_back(VBO1(0, side));
                            mesh.vertices.push_back(VBO(x[0] + du[0],         x[1] + du[1],         x[2] + du[2],         mW, mH, 2, ao.vertices.v2));
                            mesh.vertices.push_back(VBO1(0, side));
                            mesh.vertices.push_back(VBO(x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2], mW, mH, 3, ao.vertices.v3));
                            mesh.vertices.push_back(VBO1(0, side));

                            bool flip = ao.vertices.v0 + ao.vertices.v3 > ao.vertices.v1 + ao.vertices.v2;

                            if (backFace)
                            {
                                if (flip)
                                {
                                    mesh.indices.push_back(indices + 3);
                                    mesh.indices.push_back(indices + 2);
                                    mesh.indices.push_back(indices);
                                    mesh.indices.push_back(indices);
                                    mesh.indices.push_back(indices + 1);
                                    mesh.indices.push_back(indices + 3);
                                }
                                else
                                {
                                    mesh.indices.push_back(indices + 2);
                                    mesh.indices.push_back(indices);
                                    mesh.indices.push_back(indices + 1);
                                    mesh.indices.push_back(indices + 1);
                                    mesh.indices.push_back(indices + 3);
                                    mesh.indices.push_back(indices + 2);
                                }
                            }
                            else
                            {
                                if (flip) // flip triangles
                                {
                                    mesh.indices.push_back(indices);
                                    mesh.indices.push_back(indices + 2);
                                    mesh.indices.push_back(indices + 3);
                                    mesh.indices.push_back(indices + 3);
                                    mesh.indices.push_back(indices + 1);
                                    mesh.indices.push_back(indices);
                                }
                                else
                                {
                                    mesh.indices.push_back(indices + 2);
                                    mesh.indices.push_back(indices + 3);
                                    mesh.indices.push_back(indices + 1);
                                    mesh.indices.push_back(indices + 1);
                                    mesh.indices.push_back(indices);
                                    mesh.indices.push_back(indices + 2);
                                }
                            }

                            indices += 4;

                            // Add Quad
                            // quad(new Vector3f(x[0], x[1], x[2]),
                            //     new Vector3f(x[0] + du[0], x[1] + du[1], x[2] + du[2]),
                            //     new Vector3f(x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]),
                            //     new Vector3f(x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]),
                            //     w,
                            //     h,
                            //     mask[n],
                            //     backFace);
                        }
                        for (l = 0; l < h; ++l)
                            for (k = 0; k < w; ++k)
                                mask[n + k + l * CHUNK_SIZE].reset();
                        
                        i += w;
                        n += w;
                    }
            }
        }
    
    m_Stage = Stage::Built;
}

void Chunk::UploadMesh(const Mesh& mesh)
{
    GLCall(glGenVertexArrays(1, &m_VAO));
    GLCall(glGenBuffers(2, m_VBIO));

    GLCall(glBindVertexArray(m_VAO));

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBIO[0]));
    GLCall(glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(uint32_t), &mesh.vertices[0], GL_STATIC_DRAW));
    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 2 * sizeof(uint32_t), (GLvoid*)0));
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, 2 * sizeof(uint32_t), (GLvoid*)(sizeof(uint32_t))));

    m_IndicesCount = (uint32_t)mesh.indices.size();

    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VBIO[1]));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_IndicesCount * sizeof(uint32_t), &mesh.indices[0], GL_STATIC_DRAW));

    m_Stage = Stage::Uploaded;
}

void Chunk::Render(Shader* shader) const
{
    shader->SetUniform3f("u_ChunkOff", m_Coord.x, m_Coord.y, m_Coord.z);

    GLCall(glBindVertexArray(m_VAO));
    GLCall(glDrawElements(GL_TRIANGLES, m_IndicesCount, GL_UNSIGNED_INT, (void*)0));
}

uint32_t Chunk::GetNeighborBlock(Chunk* chunks[26], int x[3], int d, bool isNeighF, int v) const
{
    int neighborIndex = 0, x_ = x[0], y_ = x[1], z_ = x[2];
    if (d == 0) { neighborIndex = isNeighF ? 14 : 12; x_ = v; }
    else if (d == 1)
    {
        if (isNeighF)
        {
            if (m_Coord.y == CHUNK_MAX_HEIGHT - CHUNK_SIZE)
                return 0;

            neighborIndex = 16;
        }
        else
        {
            if (m_Coord.y == 0)
                return 0;

            neighborIndex = 10;
        }
        y_ = v;
    }
    else { neighborIndex = isNeighF ? 22 : 4; z_ = v; }

    return chunks[neighborIndex]->GetBlock(x_, y_, z_);
}

void Chunk::CalculateAO(Chunk* chunks[26], AO& ao, int x, int y, int z, int du[3], int dv[3]) const
{
    // V0 (0, 0)
    int sMV = GetAONeighborBlock(chunks, x - dv[0], y - dv[1], z - dv[2]);
    int sPV = GetAONeighborBlock(chunks, x + dv[0], y + dv[1], z + dv[2]);
    int sMU = GetAONeighborBlock(chunks, x - du[0], y - du[1], z - du[2]);
    int sPU = GetAONeighborBlock(chunks, x + du[0], y + du[1], z + du[2]);

    ao.vertices.v0 = (sMV && sMU) ? 0 : 3 - (sMV + sMU + GetAONeighborBlock(chunks, x - dv[0] - du[0], y - dv[1] - du[1], z - dv[2] - du[2]));
    ao.vertices.v1 = (sPV && sMU) ? 0 : 3 - (sPV + sMU + GetAONeighborBlock(chunks, x + dv[0] - du[0], y + dv[1] - du[1], z + dv[2] - du[2]));
    ao.vertices.v2 = (sMV && sPU) ? 0 : 3 - (sMV + sPU + GetAONeighborBlock(chunks, x - dv[0] + du[0], y - dv[1] + du[1], z - dv[2] + du[2]));
    ao.vertices.v3 = (sPV && sPU) ? 0 : 3 - (sPV + sPU + GetAONeighborBlock(chunks, x + dv[0] + du[0], y + dv[1] + du[1], z + dv[2] + du[2]));
}

int Chunk::GetAONeighborBlock(Chunk* chunks[26], int x, int y, int z) const
{
    int neighborIndx = 0;
    bool upBorder = false, downBorder = false;
    if (CoordinateInBound(&x, &y, &z, &neighborIndx, &upBorder, &downBorder))
        return m_Data[x + y * CHUNK_SIZE + z * CHUNK_SIZES];
    else if ((upBorder && m_Coord.y == CHUNK_MAX_HEIGHT - CHUNK_SIZE) ||
             (downBorder && m_Coord.y == 0))
        return 0;
    else
        return chunks[neighborIndx]->GetBlock(x, y, z);
}

bool Chunk::CoordinateInBound(int* x, int* y, int* z, int* neighborIndx, bool* upBorder, bool* downBorder) const
{
    int indx;

    if      (*x < 0)           { indx = 0; *x = CHUNK_SIZE - 1; }
    else if (*x >= CHUNK_SIZE) { indx = 2; *x = 0;              }
    else                         indx = 1;

    if      (*y < 0)           {            *y = CHUNK_SIZE - 1; *downBorder = true; }
    else if (*y >= CHUNK_SIZE) { indx += 6; *y = 0;              *upBorder   = true; } // 2 * 3
    else                         indx += 3;  // 1 * 3

    if      (*z < 0)           {             *z = CHUNK_SIZE - 1; }
    else if (*z >= CHUNK_SIZE) { indx += 18; *z = 0;              } // 2 * 9
    else                         indx += 9;  // 1 * 9

    return (*neighborIndx = indx) == 13; // 1 + 1 * 3 + 1 * 9
}
