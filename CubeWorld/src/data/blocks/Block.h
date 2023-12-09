#pragma once

#include "../tile_entities/TileEntity.h"

#include <string>
#include <vector>

#include <glm/glm.hpp>

class Block
{
public:
	uint32_t m_ID = 0, m_UVOffset = 0;
	bool m_IsTransparent = false, m_IsTranslucent = false, m_IsLiquid = false /* For Raycasting */;
	std::string m_Name;
	std::vector<glm::vec2> m_UV;

public:
	enum Side { Front, Back, Up, Down, Right, Left };

	Block(const std::string& name, std::vector<glm::vec2> uvs);

	~Block();

	inline void Register(uint32_t id, uint32_t uvOffset) { m_ID = id; m_UVOffset = uvOffset; }

	virtual bool ShouldRenderSide(Block::Side side, const Block* neighbor) const { return neighbor->m_IsTransparent && neighbor->m_ID != m_ID; }

	virtual void GetSideUV(Block::Side orientation, Block::Side side, uint32_t* id, bool* uvFlip) const { *id = m_UVOffset; *uvFlip = false; }

	virtual bool HasCustomMesh() const { return false; }
	virtual void RenderCustomMesh(std::vector<uint32_t>& data, std::vector<uint32_t>& transparent, int x, int y, int z) const {}

	virtual bool HasTileEntity() const { return false; }
	virtual TileEntity* CreateTileEntity(const glm::vec3& coord) const { return nullptr; }

	virtual bool CanBePlaced(CubeWorld* world, int x, int y, int z) const { return true; }
};

struct ChunkBlock
{
public:
	uint32_t data = 0;

public:
	inline void SetBlock(uint32_t id, Block::Side side) { data = id | (side << 29); }

	inline const uint32_t    GetID()   const { return data & 0x1FFFFFFF; }
	inline const Block::Side GetSide() const { return (Block::Side)(data >> 29); }

	bool operator==(const ChunkBlock& o)
	{
		return data == o.data;
	}

	bool operator!=(const ChunkBlock& o)
	{
		return data != o.data;
	}
};
