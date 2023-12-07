#pragma once

#include "Shader.h"

#include "utils/SimplexNoise.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>
#include <mutex>

#define CHUNK_SIZE 32
#define CHUNK_SIZES CHUNK_SIZE * CHUNK_SIZE
#define CHUNK_SIZEQ CHUNK_SIZES * CHUNK_SIZE

#define CHUNK_SIZE_INV (1.0f / CHUNK_SIZE)

#define CHUNK_Y_COUNT 8
#define CHUNK_MAX_HEIGHT CHUNK_SIZE * CHUNK_Y_COUNT

#define HCHUNK_SIZE CHUNK_SIZE * 0.5f

#define SQRT3 1.7320508f
#define SPHERE_CHUNK_RADIUS HCHUNK_SIZE * SQRT3

// X 6 Bit - Y 6 Bit - Z 6 Bit | Width 5 Bit | Heigth 5 Bit | UV 2 Bit | AO 2 Bit
#define VBO(x, y, z, w, h, uv, ao) (x) | ((y) << 6) | ((z) << 12) | ((w) << 18) | ((h) << 23) | ((uv) << 28) | ((ao) << 30)

// ID 12 Bit | Norm 3 Bit | EMPTY 20 Bit
#define VBO1(id, norm) (id) | ((norm) << 12)

struct Mesh
{
	std::vector<uint32_t> vertices;
	std::vector<uint32_t> indices;
};

enum FaceSide { Front, Back, Top, Bottom, Right, Left };

class CubeWorld;

class Chunk
{
public:
	enum Stage { Initialized, Filling, Filled, WaitingNeighbors, Building, Built, Uploaded };

	uint32_t m_IndicesCount = 0;

	glm::vec3 m_Coord;

	std::mutex m_Lock;

public:
	Chunk(const glm::vec3& coord)
		: m_Coord(coord) {}

	~Chunk();

	void Fill(SimplexNoise* noise);

	void GenerateMesh(CubeWorld* world, Mesh& mesh);
	
	void UploadMesh(const Mesh& mesh);

	void Render(Shader* shader) const;

	inline uint32_t GetBlock(int x, int y, int z) const { return m_Data[x + y * CHUNK_SIZE + z * CHUNK_SIZES]; }
	inline uint32_t GetBlock(int index)           const { return m_Data[index]; }

public:
	inline Stage GetStage()                   const { return m_Stage; }
	inline void  SetStage(const Stage& stage)       { m_Stage = stage; }
	inline bool  IsStage (const Stage& stage) const { return m_Stage == stage; }

private:
	uint32_t GetNeighborBlock(Chunk* chunks[26], int x[3], int d, bool isNeighF, int v) const;

private:
	uint32_t* m_Data = nullptr;

	Stage m_Stage = Stage::Initialized;

	uint32_t m_VAO = 0;
	uint32_t m_VBIO[2]{ 0, 0 };
};
