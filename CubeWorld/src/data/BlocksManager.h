#pragma once

#include "blocks/Block.h"

#include <vector>
#include <memory>
#include <unordered_map>

struct UniformBlock
{
	glm::uvec2 offset; // 1 uint offset + 1 padding
	glm::vec2 uv;
};

class BlocksManager
{
public:
	static std::vector<Block*> m_Blocks;
	static std::unordered_map<std::string, uint32_t> m_NamedBlocks;

	static uint32_t m_UVOffsets;

	static UniformBlock* m_UniformBlocks;

	static uint32_t m_UBO;

	static glm::vec2 m_Step;

public:
	static void Init();
	static void Dispose();

	static uint32_t RegisterBlock(Block* block);

	static void UploadBlocks();

	static void Bind(uint32_t index = 0);

	static inline Block* GetBlock(uint32_t id)
	{
		if (id < 0 || id >= m_Blocks.size())
			return nullptr;

		return m_Blocks[id];
	}

	static inline Block* GetBlock(const std::string& name)
	{
		auto it = m_NamedBlocks.find(name);
		if (it == m_NamedBlocks.end())
			return nullptr;

		return m_Blocks[it->second];
	}

	static inline Block* GetBlock(const ChunkBlock& block)
	{
		return m_Blocks[block.GetID()];
	}

	static inline uint32_t GetBlockID(const std::string& name)
	{
		auto it = m_NamedBlocks.find(name);
		if (it == m_NamedBlocks.end())
			return -1;

		return it->second;
	}

	static inline void SetAtlasStep(const glm::vec2& step) { m_Step = step; }
	static inline glm::vec2& GetAtlasStep() { return m_Step; }
};
