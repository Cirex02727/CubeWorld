#pragma once

#include "Shader.h"
#include "Camera.h"
#include "Texture.h"

#include "Chunk.h"

#include "utils/ThreadPool.h"
#include "utils/SimplexNoise.h"


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <unordered_map>
#include <memory>

struct ApplicationSpecification;

#define min(a, b) a < b ? a : b

struct WorldSettings
{
	int MaxRenderDistance = 0;
	int RenderDistance = min(5, MaxRenderDistance);

	int RenderDistanceUnload = RenderDistance + 3;

	float ChunkScale = 0.00055f;
};

struct WorldGenerationSettings
{
	float Frequency   = 4.012f,
		  Amplitude   = 2.151f,
		  Lacunarity  = 2.267f,
		  Persistence = 0.291f;
};

class CubeWorld
{
public:
	CubeWorld(ApplicationSpecification* specification)
		: m_Specification(specification) {}
	~CubeWorld();

	void Init();

	void Update(float timestep);

	void Render();

	void OnResize();

private:
	void SettupOpenGLSettings();

	void InitFramebuffer();
	void InitCrosshair();
	void InitInteract();

private:
	ApplicationSpecification* m_Specification;

	// Screen
	uint32_t m_Width = 0, m_Height = 0, m_WidthScaled = 0, m_HeightScaled = 0;

	// Framebuffer
	uint32_t m_FBO = 0, m_FBOColor = 0, m_FBODepth = 0, m_FBOQuadVAO = 0, m_FBOQuadVBO = 0;

	std::unique_ptr<Shader> m_FBOShader;

	// World
	WorldSettings m_Settings;
	WorldGenerationSettings m_GenerationSettings;

	std::unordered_map<glm::vec3, Chunk> m_Chunks;

	std::unique_ptr<ThreadPool> m_ThreadPool;
	std::unique_ptr<SimplexNoise> m_Noise;
	std::unique_ptr<Camera> m_Camera;
	std::unique_ptr<Texture> m_Texture, m_CrosshairTexture;
	std::unique_ptr<Shader> m_Shader, m_CrosshairShader, m_InteractShader;

	uint32_t m_CrosshairVAO, m_CrosshairVBO, m_InteractVAO, m_InteractVBO, m_InteractIBO;
};
