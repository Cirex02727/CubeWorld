#include "Block.h"

#include "../BlocksManager.h"

Block::Block(const std::string& name, std::vector<glm::vec2> uvs)
	: m_UV(uvs), m_Name(name)
{
	BlocksManager::RegisterBlock(this);
}

Block::~Block()
{
}
