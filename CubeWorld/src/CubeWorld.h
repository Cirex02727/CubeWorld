#pragma once

#include "Shader.h"
#include "Camera.h"
#include "Frustum.h"
#include "Texture.h"

#include "Chunk.h"

#include "utils/Timer.h"
#include "utils/ThreadPool.h"
#include "utils/SimplexNoise.h"


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <queue>

struct WindowSpecification;

#define min(a, b) a < b ? a : b

struct WorldSettings
{
	int MaxRenderDistance = 15;
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
	static std::string BytesToText(double bytes);
	static std::string FormatFloat(double n, int digit);

public:
	CubeWorld(WindowSpecification* specification)
		: m_Specification(specification) {}
	~CubeWorld();

	void Init();

	void Update(float timestep);

	void Render();

	void ImGuiRender();

	void BuildChunk(const glm::vec3& coord, bool requestMesh = true);

	void SetChunkDirty(Chunk* chunk, const glm::vec3& coord, bool isGenerating = false);

	void BuildChunkNotify(const glm::vec3& coord);

	void GenerateChunkMesh(Chunk* chunk);
	void UpdateChunkMesh(Chunk* chunk);

	bool CheckNeighborsChunks(Chunk* chunk, const glm::vec3& coord);
	bool GetChunkNeighbors(Chunk* chunk, const glm::vec3& coord, Chunk* chunks[26]);

	bool ExistChunk(const glm::vec3& coord);
	bool GetChunk(const glm::vec3& coord, Chunk** chunk);

	void OnWindowResize();

private:
	void SettupOpenGLSettings();

	void InitFramebuffer();
	void InitCrosshair();
	void InitInteract();

private:
	WindowSpecification* m_Specification;

	// Screen
	uint32_t m_Width = 0, m_Height = 0, m_WidthScaled = 0, m_HeightScaled = 0;

	// Framebuffer
	uint32_t m_FBO = 0, m_FBOColor = 0, m_FBODepth = 0, m_FBOQuadVAO = 0, m_FBOQuadVBO = 0;

	std::unique_ptr<Shader> m_FBOShader;

	// World
	WorldSettings m_Settings;
	WorldGenerationSettings m_GenerationSettings;

	std::vector<glm::vec3> m_UpdateCoord;

	std::unordered_map<glm::vec3, Chunk*> m_Chunks, m_MeshedChunks, m_GeneratingChunks;
	std::mutex m_ChunksLock, m_MeshedChunksLock, m_GeneratingChunksLock;

	std::queue<std::tuple<Chunk*, Mesh>> m_ChunksUpload;
	std::mutex m_ChunksUploadLock;

	std::queue<Chunk*> m_DirtyChunks;
	std::mutex m_DirtyChunksLock;

	std::unique_ptr<ThreadPool> m_ThreadPool;
	std::unique_ptr<SimplexNoise> m_Noise;
	std::unique_ptr<Camera> m_Camera;
	std::unique_ptr<Frustum> m_Frustum;
	std::unique_ptr<Texture> m_Texture, m_CrosshairTexture;
	std::unique_ptr<Shader> m_Shader, m_CrosshairShader, m_InteractShader;

	uint32_t m_CrosshairVAO = 0, m_CrosshairVBO = 0, m_InteractVAO = 0, m_InteractVBO = 0, m_InteractIBO = 0;

	bool m_DebugNormal = false, m_DebugUV = false;

	bool m_WorldGenerated = false;
	double m_TotalBytes = 0;

	uint16_t m_RenderedChunk = 0;

	Timer m_GenerationTimer;
};
