#pragma once

#include "Shader.h"

#include "utils/SimplexNoise.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

#define CHUNK_SIZE 32
#define CHUNK_SIZES CHUNK_SIZE * CHUNK_SIZE
#define CHUNK_SIZEQ CHUNK_SIZES * CHUNK_SIZE

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

class Chunk
{
public:
	uint32_t m_IndicesCount = 0;

public:
	Chunk(const glm::vec3& coord)
		: m_Coord(coord) {}

	~Chunk();

	void Fill(SimplexNoise* noise);

	void GenerateMesh(Mesh& mesh);
	
	void UploadMesh(Mesh& mesh);

	void Render(Shader* shader) const;

private:
	glm::vec3 m_Coord;

	uint32_t* m_Data = nullptr;

	uint32_t m_VAO = 0;
	uint32_t m_VBIO[2]{ 0, 0 };
};
