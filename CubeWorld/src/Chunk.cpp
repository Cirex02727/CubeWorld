#include "Chunk.h"

#include "Core.h"
#include "CubeWorld.h"

#include "utils/Timer.h"
#include "utils/Instrumentor.h"

#include "data/BlocksManager.h"

Chunk::~Chunk()
{
	GLCall(glDeleteVertexArrays(2, m_VAO));
	GLCall(glDeleteBuffers(4, m_VBIO));

	delete[] m_Data;
}

void Chunk::Fill(SimplexNoise* noise)
{
    m_Stage = Stage::Filling;

    m_Data = new ChunkBlock[CHUNK_SIZEQ]{};

	// Fill Density
    uint16_t* data = new uint16_t[CHUNK_SIZES];
    if (data == nullptr)
        return;

    float scale = 0.00065f;

    for (uint16_t z = 0; z < CHUNK_SIZE; ++z)
        for (uint16_t x = 0; x < CHUNK_SIZE; ++x)
            data[x + z * CHUNK_SIZE] = (uint16_t)((noise->fractal(5, (x + m_Coord.x) * scale, (z + m_Coord.z) * scale) + 1) * 0.5f * CHUNK_MAX_MOUNTAIN);

    srand(unsigned int(time(NULL)));

    const Block* dirt    = BlocksManager::GetBlock("Dirt");
    const Block* grass   = BlocksManager::GetBlock("Grass");
    const Block* stone   = BlocksManager::GetBlock("Stone");
    const Block* water   = BlocksManager::GetBlock("Water");
    const Block* sand    = BlocksManager::GetBlock("Sand");
    const Block* snow    = BlocksManager::GetBlock("Snow");
    const Block* bedrock = BlocksManager::GetBlock("Bedrock");

    const Block* block = nullptr;

    int rnd = 0;

    for (uint16_t z = 0; z < CHUNK_SIZE; ++z)
        for (uint16_t x = 0; x < CHUNK_SIZE; ++x)
        {
            int maxH = data[x + z * CHUNK_SIZE];

            rnd = rand() % 100;

            for (int y = 0; y < CHUNK_SIZE; y++)
            {
                int wY = y + (int)m_Coord.y;
                if (wY > std::max(maxH, CHUNK_WATER_HEIGHT))
                    break;

                if (wY == 0)
                    block = bedrock;

                else if (wY <= CHUNK_WATER_HEIGHT + 2 && wY >= maxH - 2 && wY <= maxH)
                    block = sand;

                else if (wY >= CHUNK_SNOW_HEIGHT + 12 && wY >= maxH - 1 && wY <= maxH)
                    block = snow;
                else if (wY >= CHUNK_SNOW_HEIGHT + 9 && wY < CHUNK_SNOW_HEIGHT + 12 && rnd > 15 && wY >= maxH - 1 && wY <= maxH)
                    block = snow;
                else if (wY >= CHUNK_SNOW_HEIGHT + 4 && wY < CHUNK_SNOW_HEIGHT + 9 && rnd > 35 && wY >= maxH - 1 && wY <= maxH)
                    block = snow;
                else if (wY >= CHUNK_SNOW_HEIGHT && wY < CHUNK_SNOW_HEIGHT + 4 && rnd > 55 && wY >= maxH - 1 && wY <= maxH)
                    block = snow;

                else if (wY >= CHUNK_STONE_HEIGHT && wY <= maxH)
                    block = stone;

                else if (wY <= maxH - 5)
                    block = stone;
                else if (wY <= maxH - 1)
                    block = dirt;
                else if (wY <= maxH)
                    block = grass;

                else if (wY <= CHUNK_WATER_HEIGHT)
                    block = water;

                else
                    continue;

                m_Data[ID(x, y, z)].SetBlock(block->m_ID, Block::Side::Front);
                block = nullptr;
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

    FaceMask mask[CHUNK_SIZES];
    memset(mask, 0, CHUNK_SIZES * sizeof(FaceMask));

    ChunkBlock voxelFace, voxelFace1;
    Block* blockFace, *blockFace1;

    uint32_t indicesCount = 0, indicesTCount = 0;

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
                        voxelFace  = (x[d] >= 0)             ? m_Data[ID(x[0], x[1], x[2])]
                            : GetNeighborBlock(chunks, x, d, false, CHUNK_SIZE - 1);
                        voxelFace1 = (x[d] < CHUNK_SIZE - 1) ? m_Data[ID(x[0] + q[0], x[1] + q[1], x[2] + q[2])]
                            : GetNeighborBlock(chunks, x, d, true,  0);

                        if (voxelFace == voxelFace1)
                        {
                            mask[n].ao.data      = 0x03030303;
                            mask[n++].block.data = 0;
                            continue;
                        }

                        if (backFace)
                        {
                            dt[0] = x[0];
                            dt[1] = x[1];
                            dt[2] = x[2];

                            ChunkBlock temp = voxelFace1;
                            voxelFace1 = voxelFace;
                            voxelFace = temp;
                        }
                        else
                        {
                            dt[0] = x[0] + q[0];
                            dt[1] = x[1] + q[1];
                            dt[2] = x[2] + q[2];
                        }
                        
                        blockFace  = BlocksManager::GetBlock(voxelFace);
                        blockFace1 = BlocksManager::GetBlock(voxelFace1);
                        if (!blockFace1->m_IsTransparent)
                        {
                            mask[n].ao.data = 0x03030303;
                            mask[n++].block.data = 0;
                            continue;
                        }

                        CalculateAO(chunks, mask[n].ao, dt[0], dt[1], dt[2], du, dv);

                        mask[n++].block = std::move(voxelFace);
                    }

                ++x[d];

                n = 0;
                for (j = 0; j < CHUNK_SIZE; ++j)
                    for (i = 0; i < CHUNK_SIZE;)
                    {
                        if (!mask[n].block.data)
                        {
                            ++i;
                            ++n;
                            continue;
                        }

                        for (w = 1; i + w < CHUNK_SIZE && mask[n + w].block.data && mask[n + w] == mask[n]; ++w);

                        bool done = false;

                        for (h = 1; j + h < CHUNK_SIZE; ++h)
                        {
                            for (k = 0; k < w; ++k)
                            {
                                int indx = n + k + h * CHUNK_SIZE;
                                if (!mask[indx].block.data || mask[indx] != mask[n]) { done = true; break; }
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

                            const ChunkBlock& chunkBlock = mask[n].block;
                            const AO& ao = mask[n].ao;

                            int mW = h - 1, mH = w - 1;

                            const uint32_t id = chunkBlock.GetID();
                            const Block* block = BlocksManager::GetBlock(id);

                            bool isFaceFlip = d != 0;
                            if (isFaceFlip) // Is Flip
                            {
                                int temp = mH;
                                mH = mW;
                                mW = temp;
                            }

                            bool isTranslucent = block->m_IsTranslucent;

                            std::vector<uint32_t>& vertices = isTranslucent ? mesh.tvertices : mesh.vertices;
                            std::vector<uint32_t>& indices  = isTranslucent ? mesh.tindices  : mesh.indices;

                            uint32_t& verticesCount = isTranslucent ? indicesTCount : indicesCount;

                            vertices.push_back(VBO(x[0],                 x[1],                 x[2],                 mW, mH, 0, ao.vertices.v0));
                            vertices.push_back(VBO1(id, side));

                            if (isFaceFlip)
                            {
                                vertices.push_back(VBO(x[0] + dv[0], x[1] + dv[1], x[2] + dv[2], mW, mH, 2, ao.vertices.v1));
                                vertices.push_back(VBO1(id, side));
                                vertices.push_back(VBO(x[0] + du[0], x[1] + du[1], x[2] + du[2], mW, mH, 1, ao.vertices.v2));
                                vertices.push_back(VBO1(id, side));
                            }
                            else
                            {
                                vertices.push_back(VBO(x[0] + dv[0], x[1] + dv[1], x[2] + dv[2], mW, mH, 1, ao.vertices.v1));
                                vertices.push_back(VBO1(id, side));
                                vertices.push_back(VBO(x[0] + du[0], x[1] + du[1], x[2] + du[2], mW, mH, 2, ao.vertices.v2));
                                vertices.push_back(VBO1(id, side));
                            }

                            vertices.push_back(VBO(x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2], mW, mH, 3, ao.vertices.v3));
                            vertices.push_back(VBO1(id, side));

                            bool flip = ao.vertices.v0 + ao.vertices.v3 > ao.vertices.v1 + ao.vertices.v2;

                            if (backFace)
                            {
                                if (flip)
                                {
                                    indices.push_back(verticesCount + 3);
                                    indices.push_back(verticesCount + 2);
                                    indices.push_back(verticesCount);
                                    indices.push_back(verticesCount);
                                    indices.push_back(verticesCount + 1);
                                    indices.push_back(verticesCount + 3);
                                }
                                else
                                {
                                    indices.push_back(verticesCount + 2);
                                    indices.push_back(verticesCount);
                                    indices.push_back(verticesCount + 1);
                                    indices.push_back(verticesCount + 1);
                                    indices.push_back(verticesCount + 3);
                                    indices.push_back(verticesCount + 2);
                                }
                            }
                            else if (flip) // flip triangles
                            {
                                indices.push_back(verticesCount);
                                indices.push_back(verticesCount + 2);
                                indices.push_back(verticesCount + 3);
                                indices.push_back(verticesCount + 3);
                                indices.push_back(verticesCount + 1);
                                indices.push_back(verticesCount);
                            }
                            else
                            {
                                indices.push_back(verticesCount + 2);
                                indices.push_back(verticesCount + 3);
                                indices.push_back(verticesCount + 1);
                                indices.push_back(verticesCount + 1);
                                indices.push_back(verticesCount);
                                indices.push_back(verticesCount + 2);
                            }

                            verticesCount += 4;
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
    if (m_Stage != Stage::Built && m_Stage != Stage::Uploaded)
        return;

    m_IndicesCount = (uint32_t)mesh.indices.size();
    if (m_IndicesCount > 0)
    {
        if (m_BufferSize == 0) // Create new buffer
        {
            GLCall(glGenVertexArrays(1, m_VAO));
            GLCall(glGenBuffers(2, m_VBIO));

            // Opaque
            GLCall(glBindVertexArray(m_VAO[0]));

            m_BufferSize = (uint32_t)mesh.vertices.size();

            GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBIO[0]));
            GLCall(glBufferData(GL_ARRAY_BUFFER, m_BufferSize * sizeof(uint32_t), &mesh.vertices[0], GL_STATIC_DRAW));
            GLCall(glEnableVertexAttribArray(0));
            GLCall(glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 2 * sizeof(uint32_t), (GLvoid*)0));
            GLCall(glEnableVertexAttribArray(1));
            GLCall(glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, 2 * sizeof(uint32_t), (GLvoid*)(sizeof(uint32_t))));

            GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VBIO[1]));
            GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_IndicesCount * sizeof(uint32_t), &mesh.indices[0], GL_STATIC_DRAW));
        }
        else if (m_BufferSize < mesh.vertices.size())
        {
            m_BufferSize = (uint32_t)mesh.vertices.size();

            GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBIO[0]));
            GLCall(glBufferData(GL_ARRAY_BUFFER, m_BufferSize * sizeof(uint32_t), &mesh.vertices[0], GL_STATIC_DRAW));

            GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VBIO[1]));
            GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_IndicesCount * sizeof(uint32_t), &mesh.indices[0], GL_STATIC_DRAW));
        }
        else
        {
            GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBIO[0]));
            GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.vertices.size() * sizeof(uint32_t), &mesh.vertices[0]));

            GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VBIO[1]));
            GLCall(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, mesh.indices.size() * sizeof(uint32_t), &mesh.indices[0]));
        }
    }


    // Translucent
    m_TIndicesCount = (uint32_t)mesh.tindices.size();
    if (m_TIndicesCount > 0)
    {
        if (m_TBufferSize == 0) // Create new buffer
        {
            GLCall(glGenVertexArrays(1, m_VAO + 1));
            GLCall(glGenBuffers(2, m_VBIO + 2));

            // Opaque
            GLCall(glBindVertexArray(m_VAO[1]));

            m_TBufferSize = (uint32_t)mesh.tvertices.size();

            GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBIO[2]));
            GLCall(glBufferData(GL_ARRAY_BUFFER, m_TBufferSize * sizeof(uint32_t), &mesh.tvertices[0], GL_STATIC_DRAW));
            GLCall(glEnableVertexAttribArray(0));
            GLCall(glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 2 * sizeof(uint32_t), (GLvoid*)0));
            GLCall(glEnableVertexAttribArray(1));
            GLCall(glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, 2 * sizeof(uint32_t), (GLvoid*)(sizeof(uint32_t))));

            GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VBIO[3]));
            GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_TIndicesCount * sizeof(uint32_t), &mesh.tindices[0], GL_STATIC_DRAW));
        }
        else if (m_TBufferSize < mesh.tvertices.size())
        {
            m_TBufferSize = (uint32_t)mesh.tvertices.size();

            GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBIO[2]));
            GLCall(glBufferData(GL_ARRAY_BUFFER, m_TBufferSize * sizeof(uint32_t), &mesh.tvertices[0], GL_STATIC_DRAW));

            GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VBIO[3]));
            GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_TIndicesCount * sizeof(uint32_t), &mesh.tindices[0], GL_STATIC_DRAW));
        }
        else
        {
            GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBIO[2]));
            GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.tvertices.size() * sizeof(uint32_t), &mesh.tvertices[0]));

            GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_VBIO[3]));
            GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.tindices.size() * sizeof(uint32_t), &mesh.tindices[0]));
        }
    }

    m_Stage = Stage::Uploaded;
}

void Chunk::Render(Shader* shader) const
{
    if (m_IndicesCount == 0)
        return;

    shader->SetUniform3f("u_ChunkOff", m_Coord.x, m_Coord.y, m_Coord.z);

    GLCall(glBindVertexArray(m_VAO[0]));
    GLCall(glDrawElements(GL_TRIANGLES, m_IndicesCount, GL_UNSIGNED_INT, (void*)0));
}

void Chunk::RenderT(Shader* shader) const
{
    if (m_TIndicesCount == 0)
        return;

    shader->SetUniform3f("u_ChunkOff", m_Coord.x, m_Coord.y, m_Coord.z);

    GLCall(glBindVertexArray(m_VAO[1]));
    GLCall(glDrawElements(GL_TRIANGLES, m_TIndicesCount, GL_UNSIGNED_INT, (void*)0));
}

ChunkBlock Chunk::GetNeighborBlock(Chunk* chunks[26], int x[3], int d, bool isNeighF, int v) const
{
    int neighborIndex = 0, x_ = x[0], y_ = x[1], z_ = x[2];
    if (d == 0) { neighborIndex = isNeighF ? 14 : 12; x_ = v; }
    else if (d == 1)
    {
        if (isNeighF)
        {
            if (m_Coord.y == CHUNK_MAX_HEIGHT - CHUNK_SIZE)
                return ChunkBlock{};

            neighborIndex = 16;
        }
        else
        {
            if (m_Coord.y == 0)
                return ChunkBlock{};

            neighborIndex = 10;
        }
        y_ = v;
    }
    else { neighborIndex = isNeighF ? 22 : 4; z_ = v; }

    return chunks[neighborIndex]->GetBlock(ID(x_, y_, z_));
}

void Chunk::RemoveTileEntity(const glm::vec3& coord)
{
    m_TileEntitiesToRemove.push(coord);
}

void Chunk::Update(CubeWorld* world)
{
    for (auto& [_, tileEntity] : m_TileEntities)
        tileEntity->Update(world);
}

void Chunk::PostUpdate()
{
    while (m_TileEntitiesToRemove.size() > 0)
    {
        const glm::vec3& coord = m_TileEntitiesToRemove.front();
        m_TileEntities.erase(coord);

        m_TileEntitiesToRemove.pop();
    }
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
        return !BlocksManager::GetBlock(m_Data[ID(x, y, z)])->m_IsTransparent;
    else if ((upBorder && m_Coord.y == CHUNK_MAX_HEIGHT - CHUNK_SIZE) ||
             (downBorder && m_Coord.y == 0))
        return 0;
    else
        return !BlocksManager::GetBlock(chunks[neighborIndx]->GetBlock(ID(x, y, z)))->m_IsTransparent;
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
