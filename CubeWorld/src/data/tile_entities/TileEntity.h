#pragma once

#include <glm/glm.hpp>

class Block;
class CubeWorld;

class TileEntity
{
public:
	TileEntity(const Block* baseBlock, const glm::vec3& coord)
		: m_BaseBlock(baseBlock), m_Coord(coord) {}

	virtual void Update(CubeWorld* world) {}

public:
	const Block* m_BaseBlock;
	glm::vec3 m_Coord;
};
