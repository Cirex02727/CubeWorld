#include "BlocksManager.h"

#include "Core.h"

std::vector<Block*> BlocksManager::m_Blocks{};
std::unordered_map<std::string, uint32_t> BlocksManager::m_NamedBlocks{};

uint32_t BlocksManager::m_UVOffsets = 0;

UniformBlock* BlocksManager::m_UniformBlocks = nullptr;

uint32_t BlocksManager::m_UBO = 0;


void BlocksManager::Init()
{
	m_UniformBlocks = (UniformBlock*)calloc(4096, sizeof(UniformBlock));;

	GLCall(glGenBuffers(1, &m_UBO));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_UBO));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, 4096 * sizeof(UniformBlock), nullptr, GL_DYNAMIC_DRAW));
}

void BlocksManager::Dispose()
{
	if (m_UBO)
		GLCall(glDeleteBuffers(1, &m_UBO));

	for (Block* block : m_Blocks)
		free(block);

	free(m_UniformBlocks);
}

uint32_t BlocksManager::RegisterBlock(Block* block)
{
	uint32_t id = (uint32_t)m_Blocks.size();
	block->Register(id, m_UVOffsets);
	m_UVOffsets += (uint32_t)block->m_UV.size();
	m_NamedBlocks[block->m_Name] = id;
	m_Blocks.push_back(block);
	return id;
}

void BlocksManager::UploadBlocks()
{
	uint32_t i = 0;
	for (const Block* block : m_Blocks)
	{
		for (const glm::vec2& uv : block->m_UV)
		{
			m_UniformBlocks[i] = { { i, 0 }, uv };
			++i;
		}
	}

	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_UBO));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, i * sizeof(UniformBlock), m_UniformBlocks));
}

void BlocksManager::Bind(uint32_t index)
{
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, index, m_UBO));
}
