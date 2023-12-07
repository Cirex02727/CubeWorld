#include "CubeWorld.h"

#include "Core.h"

#include "utils/Timer.h"
#include "utils/Benchmark.h"

#include "Application.h"

glm::vec3& border_vec3(glm::vec3& v, int d)
{
	switch (d)
	{
	case 0: v.x = CHUNK_SIZE - 1; break;
	case 1: v.y = CHUNK_SIZE - 1; break;
	case 2: v.z = CHUNK_SIZE - 1; break;
	}
	return v;
}

inline glm::vec3& border_vec3_1(glm::vec3& v, int d)
{
	switch (d)
	{
	case 0: v.x = CHUNK_SIZE - 1; break;
	case 1: v.y = CHUNK_SIZE - 1; break;
	case 2: v.z = CHUNK_SIZE - 1; break;
	}
	return v;
}

int neighbor_index(int d, bool backface)
{
	switch (d)
	{
	case 0:  return backface ? 0 : 1;
	case 1:  return backface ? 2 : 3;
	case 2:  return backface ? 4 : 5;
	default: return -1;
	}
}

inline int neighbor_index_1(int d, bool backface)
{
	switch (d)
	{
	case 0:  return backface ? 0 : 1;
	case 1:  return backface ? 2 : 3;
	case 2:  return backface ? 4 : 5;
	default: return -1;
	}
}

void CubeWorld::Init()
{
	bool backface = true;
	int d = 1;

	glm::vec3 v{};

	uint32_t neighboars[26]{};

	int count = 1'000;

	Benchmark::Bench(count, [&]()
	{
		uint32_t chunk = neighboars[neighbor_index(d, backface)];
		border_vec3(v, d);
	});

	Benchmark::Bench(count, [&]()
	{
		uint32_t chunk = neighboars[neighbor_index_1(d, backface)];
		border_vec3(v, d);
	});

	Benchmark::Bench(count, [&]()
	{
		uint32_t chunk = neighboars[neighbor_index(d, backface)];
		border_vec3_1(v, d);
	});

	Benchmark::Bench(count, [&]()
	{
		uint32_t chunk = neighboars[neighbor_index_1(d, backface)];
		border_vec3_1(v, d);
	});


	SettupOpenGLSettings();

	InitFramebuffer();
	InitCrosshair();
	InitInteract();

	m_ThreadPool = std::make_unique<ThreadPool>(std::thread::hardware_concurrency() - 1);
	
	m_Noise = std::make_unique<SimplexNoise>(m_GenerationSettings.Frequency, m_GenerationSettings.Amplitude, m_GenerationSettings.Lacunarity, m_GenerationSettings.Persistence);

	m_Camera = std::make_unique<Camera>(60.0f, 0.05f, 3000.0f);
	m_Camera->SetPosition({ -35.6f, 150.0f, 29.3f });
	m_Camera->SetDirection({ 0.6f, -0.4f, 0.7f });
	m_Camera->WindowResize(m_WidthScaled, m_HeightScaled);
	m_Camera->RecalculateView();

	m_Frustum = std::make_unique<Frustum>();

	m_Texture = std::make_unique<Texture>("res/textures/terrain.png", GL_NEAREST, GL_NEAREST, true, true, true);

	m_Shader = std::make_unique<Shader>("res/shaders/terrain.shader");
	m_Shader->Bind();
	m_Shader->SetUniform1i("u_Texture", 0);

	m_GenerationTimer.Reset();

	int i = 0, renderDist = m_Settings.RenderDistance, maxRenderDist = m_Settings.MaxRenderDistance;
	m_UpdateCoord.resize((size_t)(renderDist * 2 + 1) * (size_t)(renderDist * 2 + 1) * CHUNK_Y_COUNT);
	for (int z = -renderDist; z <= renderDist; z++)
		for (int y = 0; y < CHUNK_Y_COUNT; y++)
			for (int x = -renderDist; x <= renderDist; x++)
				m_UpdateCoord[i++] = { x, y, z };

	// To Implement
	// Neighbors/Update Coords
	// BlocksManager (UploadBlocks)
	// StructureManager (Load Structures)
	// Frustum
	// DataCompressManager
	// Custome Blocks (ShaderCustome)
	// BuildChunks
	// Load Mods
}

void CubeWorld::SettupOpenGLSettings()
{
	GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

	GLCall(glEnable(GL_DEPTH_TEST));

	GLCall(glEnable(GL_CULL_FACE));
	GLCall(glCullFace(GL_BACK));

	// GLCall(glEnable(GL_MULTISAMPLE));

	bool VSync = false;
	GLCall(glfwSwapInterval(VSync));
}

void CubeWorld::InitFramebuffer()
{
	m_Width = m_Specification->Width;
	m_Height = m_Specification->Height;

	m_WidthScaled = (uint32_t)(m_Width * m_Specification->ScreenScaleFactor);
	m_HeightScaled = (uint32_t)(m_Height * m_Specification->ScreenScaleFactor);


	GLCall(glGenFramebuffers(1, &m_FBO));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FBO));

	GLCall(glGenTextures(1, &m_FBOColor));
	GLCall(glBindTexture(GL_TEXTURE_2D, m_FBOColor));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_WidthScaled, m_HeightScaled, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_FBOColor, 0));

	GLCall(glGenRenderbuffers(1, &m_FBODepth));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_FBODepth));
	GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_WidthScaled, m_HeightScaled));

	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_FBODepth));


	{
		GLCall(uint32_t status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << "Framebuffer failed to generate!" << std::endl;
			std::exit(status);
		}
	}


	// Frame Buffer Quad
	m_FBOShader = std::make_unique<Shader>("res/shaders/uv.shader");

	float quad[] =
	{
		 1.0f, -1.0f, 1.0f, 0.0f,
		 1.0f,  1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f,

		-1.0f, -1.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 1.0f
	};

	GLCall(glGenVertexArrays(1, &m_FBOQuadVAO));
	GLCall(glBindVertexArray(m_FBOQuadVAO));

	GLCall(glGenBuffers(1, &m_FBOQuadVBO));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_FBOQuadVBO));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW));

	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)0));
	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(2 * sizeof(float))));

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void CubeWorld::InitCrosshair()
{
	m_CrosshairTexture = std::make_unique<Texture>("res/textures/crosshair.png", GL_NEAREST, GL_NEAREST);

	m_CrosshairShader = std::make_unique<Shader>("res/shaders/crosshair.shader");
	m_CrosshairShader->Bind();
	m_CrosshairShader->SetUniform1i("u_Texture", 0);

	// Crosshair
	{
		const float crosshairSize = 85.0f, halfCrosshairSize = crosshairSize / 2;
		const glm::vec2 center = { m_WidthScaled / 2, m_HeightScaled / 2 };
		float positions[] = {
			center.x + halfCrosshairSize, center.y - halfCrosshairSize, 1.0f, 0.0f,
			center.x + halfCrosshairSize, center.y + halfCrosshairSize, 1.0f, 1.0f,
			center.x - halfCrosshairSize, center.y - halfCrosshairSize, 0.0f, 0.0f,
			center.x - halfCrosshairSize, center.y + halfCrosshairSize, 0.0f, 1.0f
		};

		GLCall(glGenVertexArrays(1, &m_CrosshairVAO));
		GLCall(glBindVertexArray(m_CrosshairVAO));

		GLCall(GLCall(glGenBuffers(1, &m_CrosshairVBO)));
		GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_CrosshairVBO));
		GLCall(glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(float), positions, GL_STATIC_DRAW));

		GLCall(glEnableVertexAttribArray(0));
		GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0));
		GLCall(glEnableVertexAttribArray(1));
		GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float))));
	}
}

void CubeWorld::InitInteract()
{
	m_InteractShader = std::make_unique<Shader>("res/shaders/interact.shader");
	m_InteractShader->Bind();

	float vertices[] =
	{
		// Z+
		 0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
		-0.5f,  0.5f,  0.5f, -0.5f,  0.5f,

		// Z-
		-0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
		 0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f, -0.5f,  0.5f,

		// Y+
		-0.5f,  0.5f,  0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f, -0.5f,  0.5f,

		// Y-
		-0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f, -0.5f,  0.5f,

		// X+
		 0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f,  0.5f, -0.5f,  0.5f,

		// X-
		-0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
	};

	uint32_t indices[] =
	{
		 0,  1,  2,  2,  1,  3,
		 4,  5,  6,  6,  5,  7,
		 8,  9, 10, 10,  9, 11,
		12, 13, 14, 14, 13, 15,
		16, 17, 18, 18, 17, 19,
		20, 21, 22, 22, 21, 23
	};

	GLCall(glGenVertexArrays(1, &m_InteractVAO));
	GLCall(glBindVertexArray(m_InteractVAO));

	GLCall(GLCall(glGenBuffers(1, &m_InteractVBO)));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_InteractVBO));
	GLCall(glBufferData(GL_ARRAY_BUFFER, 6 * 4 * 5 * sizeof(float), vertices, GL_STATIC_DRAW));

	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));

	GLCall(GLCall(glGenBuffers(1, &m_InteractIBO)));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_InteractIBO));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * 6 * sizeof(uint32_t), indices, GL_STATIC_DRAW));
}

CubeWorld::~CubeWorld()
{
	GLCall(glDeleteVertexArrays(1, &m_CrosshairVAO));
	GLCall(glDeleteBuffers(1, &m_CrosshairVBO));

	GLCall(glDeleteVertexArrays(1, &m_InteractVAO));
	GLCall(glDeleteBuffers(1, &m_InteractVBO));
	GLCall(glDeleteBuffers(1, &m_InteractIBO));

	GLCall(glDeleteVertexArrays(1, &m_FBOQuadVAO));
	GLCall(glDeleteBuffers(1, &m_FBOQuadVBO));

	GLCall(glDeleteTextures(1, &m_FBOColor));
	GLCall(glDeleteRenderbuffers(1, &m_FBODepth));
	GLCall(glDeleteFramebuffers(1, &m_FBO));
}

void CubeWorld::Update(float timestep)
{
	if (Input::IsKeyDown(KeyCode::H))
		m_Settings.MaxRenderDistance++;

	if (Input::IsKeyDown(KeyCode::G))
		m_Settings.MaxRenderDistance--;

	const glm::vec3 cameraChunk = glm::floor(m_Camera->GetPosition() * CHUNK_SIZE_INV);

	{
		int renderDist = m_Settings.RenderDistance, maxRenderDist = m_Settings.MaxRenderDistance;

		if (m_GeneratingChunks.size() < 100 && std::abs(maxRenderDist - renderDist) > 0)
		{
			m_Settings.RenderDistance += glm::sign(maxRenderDist - renderDist);

			int i = 0;
			m_UpdateCoord.resize((size_t)(renderDist * 2 + 1) * (size_t)(renderDist * 2 + 1) * CHUNK_Y_COUNT);
			for (int z = -renderDist; z <= renderDist; z++)
				for (int y = 0; y < CHUNK_Y_COUNT; y++)
					for (int x = -renderDist; x <= renderDist; x++)
						m_UpdateCoord[i++] = { x, y, z };
		}
	}

	for (const glm::vec3& updateCoord : m_UpdateCoord)
	{
		const glm::vec3 coord = glm::vec3{
				(updateCoord.x + cameraChunk.x) * CHUNK_SIZE,
				 updateCoord.y                  * CHUNK_SIZE,
				(updateCoord.z + cameraChunk.z) * CHUNK_SIZE };
		BuildChunk(coord);
	}


	m_Camera->OnUpdate(timestep);


	uint16_t uploaded = 0;
	while (m_ChunksUpload.size() > 0 && uploaded < 15)
	{
		const auto& [chunk, mesh] = m_ChunksUpload.front();

		// I take the chunk and if it has a mesh I upload it to the GPU
		const glm::vec3& coord = chunk->m_Coord;
		{
			size_t vertices = mesh.vertices.size();
			if (vertices <= 0)
			{
				std::lock_guard<std::mutex> uploadL(m_ChunksUploadLock);

				m_ChunksUpload.pop();
				continue;
			}

			chunk->UploadMesh(mesh);


			std::lock_guard<std::mutex> chunkL(m_MeshedChunksLock);

			if (!m_MeshedChunks.contains(coord))
			{
				m_MeshedChunks[coord] = chunk;
				m_TotalBytes += vertices * sizeof(uint32_t);
			}
		}

		{
			std::lock_guard<std::mutex> generatingL(m_GeneratingChunksLock);
			m_GeneratingChunks.erase(coord);
		}

		{
			std::lock_guard<std::mutex> uploadL(m_ChunksUploadLock);
			m_ChunksUpload.pop();
		}

		uploaded++;
	}

	//if (uploaded > 0)
	//	std::cout << "Chunks Uploaded: " << uploaded << std::endl;

	if (m_Settings.RenderDistance == m_Settings.MaxRenderDistance && m_ThreadPool->GetTaksCount() <= 0 && m_ChunksUpload.size() == 0 && !m_WorldGenerated)
	{
		m_WorldGenerated = true;
		std::cout << "World Generation in " << m_GenerationTimer.ElapsedMillis() << " ms" << std::endl;
	}


	{
		std::lock_guard<std::mutex> dirtyLock(m_DirtyChunksLock);

		while (m_DirtyChunks.size() > 0)
		{
			Chunk* chunk = m_DirtyChunks.front();

			UpdateChunkMesh(chunk);

			m_DirtyChunks.pop();
		}
	}
}

void CubeWorld::Render()
{
	GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const glm::mat4& VP = m_Camera->GetViewProjection();

	m_Shader->Bind();
	m_Shader->SetUniformMat4f("u_VP", VP);

	m_Texture->Bind();

	const glm::vec3& camPos = m_Camera->GetPosition();
	m_Shader->SetUniform3fv("u_CamPos", 1, &camPos.x);

	m_Shader->SetUniform1i("u_DebugNormal", m_DebugNormal);
	m_Shader->SetUniform1i("u_DebugUV", m_DebugUV);

	m_Frustum->Update(m_Camera.get());

	m_RenderedChunk = 0;

	{
		std::lock_guard<std::mutex> chunksL(m_MeshedChunksLock);

		Shader* shader = m_Shader.get();
		for (const auto& [coord, chunk] : m_MeshedChunks)
		{
			if (!m_Frustum->SphereIntersect(coord + HCHUNK_SIZE, SPHERE_CHUNK_RADIUS))
				continue;

			chunk->Render(shader);
			++m_RenderedChunk;
		}
	}
}

void CubeWorld::ImGuiRender()
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

	if (ImGui::SliderInt("Max FPS", &m_Specification->MaxFps, 25, 240))
		m_Specification->MaxDeltaTime = 1.0f / m_Specification->MaxFps;

	const glm::vec3& camPos = m_Camera->GetPosition();
	ImGui::Text("Camera Position: %.1f, %.1f, %.1f", camPos.x, camPos.y, camPos.z);
	const glm::vec3& camDir = m_Camera->GetDirection();
	ImGui::Text("Camera Direction: %.1f, %.1f, %.1f", camDir.x, camDir.y, camDir.z);

	ImGui::Text("Render Distance: %d/%d", m_Settings.RenderDistance, m_Settings.MaxRenderDistance);


	ImGui::Text("Chunks: %d/%d/%d", m_RenderedChunk, m_MeshedChunks.size(), m_Chunks.size());

	ImGui::Text("Generating Chunks: %d", m_GeneratingChunks.size());

	ImGui::Text("RAM Used: %s",  BytesToText((double)(m_Chunks.size() * CHUNK_SIZEQ * sizeof(uint32_t))).c_str());
	ImGui::Text("VRAM Used: %s", BytesToText(m_TotalBytes).c_str());

	ImGui::Checkbox("Debug Normal: ", &m_DebugNormal);
	if (m_DebugNormal) m_DebugUV = false;
	ImGui::Checkbox("Debug UV: ", &m_DebugUV);
	if (m_DebugUV) m_DebugNormal = false;
}

void CubeWorld::BuildChunk(const glm::vec3& coord, bool requestMesh)
{
	// Is Chunk Generating/Just Generated? If so then do not create new one
	{
		std::lock_guard<std::mutex> generatingL(m_GeneratingChunksLock);

		if (m_GeneratingChunks.contains(coord))
			return;
	}

	Chunk* chunk;
	if (GetChunk(coord, &chunk))
	{
		std::lock_guard<std::mutex> chunkL(chunk->m_Lock);

		if (chunk->IsStage(Chunk::Stage::Filled) && requestMesh)
			SetChunkDirty(chunk, coord);

		return;
	}

	{
		std::lock_guard<std::mutex> generatingL(m_GeneratingChunksLock);
		m_GeneratingChunks[coord] = chunk;
	}

	m_ThreadPool->enqueue([&, coord, requestMesh]()
	{
		Chunk* chunk = new Chunk{ coord };
		chunk->Fill(m_Noise.get());

		{
			std::lock_guard<std::mutex> chunkL(m_ChunksLock);
			m_Chunks[coord] = chunk;
		}

		if (!requestMesh)
		{
			{
				std::lock_guard<std::mutex> generatingL(m_GeneratingChunksLock);
				m_GeneratingChunks.erase(coord);
			}

			BuildChunkNotify(coord);
			return;
		}
		else
			BuildChunkNotify(coord);

		SetChunkDirty(chunk, coord, true);
	});
}

void CubeWorld::SetChunkDirty(Chunk* chunk, const glm::vec3& coord, bool isGenerating)
{
	if(!isGenerating)
	{
		std::lock_guard<std::mutex> generationL(m_GeneratingChunksLock);
		if (!m_GeneratingChunks.insert({ coord, chunk }).second)
			return;
	}

	{
		std::lock_guard<std::mutex> dirtyLock(m_DirtyChunksLock);
		m_DirtyChunks.push(chunk);
	}
}

void CubeWorld::BuildChunkNotify(const glm::vec3& coord)
{
	const glm::vec3 max{ coord.x + CHUNK_SIZE, coord.y + CHUNK_SIZE, coord.z + CHUNK_SIZE };

	glm::vec3 nCoord{};

	Chunk* chunk;

	for (nCoord.z = coord.z - CHUNK_SIZE; nCoord.z <= max.z; nCoord.z += CHUNK_SIZE)
		for (nCoord.x = coord.x - CHUNK_SIZE; nCoord.x <= max.x; nCoord.x += CHUNK_SIZE)
			for (nCoord.y = coord.y - CHUNK_SIZE; nCoord.y <= max.y; nCoord.y += CHUNK_SIZE)
			{
				if (nCoord.y < 0 || nCoord.y >= CHUNK_MAX_HEIGHT || (coord.x == nCoord.x && coord.y == nCoord.y && coord.z == nCoord.z)
					|| !GetChunk(nCoord, &chunk))
					continue;

				std::lock_guard<std::mutex> lock(chunk->m_Lock);

				if (chunk->IsStage(Chunk::Stage::WaitingNeighbors))
					SetChunkDirty(chunk, nCoord);
			}
}

void CubeWorld::GenerateChunkMesh(Chunk* chunk)
{
	Mesh mesh;
	chunk->GenerateMesh(this, mesh);

	if (mesh.vertices.size() > 0)
	{
		std::lock_guard<std::mutex> lock(m_ChunksUploadLock);

		m_ChunksUpload.push({ chunk, std::move(mesh) });
	}
	else
	{
		chunk->SetStage(Chunk::Stage::Uploaded);

		std::lock_guard<std::mutex> generatingL(m_GeneratingChunksLock);
		m_GeneratingChunks.erase(chunk->m_Coord);
	}
}

void CubeWorld::UpdateChunkMesh(Chunk* chunk)
{
	const glm::vec3& coord = chunk->m_Coord;
	if (!CheckNeighborsChunks(chunk, coord))
	{
		std::lock_guard<std::mutex> generatingL(m_GeneratingChunksLock);
		m_GeneratingChunks.erase(coord);
		return;
	}

	{
		std::lock_guard<std::mutex> generatingL(m_GeneratingChunksLock);
		if (!m_GeneratingChunks.contains(coord))
		{
			int a = 0;
		}
	}

	m_ThreadPool->enqueue([&, chunk]() { GenerateChunkMesh(chunk); });
}

bool CubeWorld::CheckNeighborsChunks(Chunk* chunk, const glm::vec3& coord)
{
	const glm::vec3 max{ coord.x + CHUNK_SIZE, coord.y + CHUNK_SIZE, coord.z + CHUNK_SIZE };

	glm::vec3 nCoord{};

	bool allNeighbors = true, firstMiss = true;

	for (nCoord.z = coord.z - CHUNK_SIZE; nCoord.z <= max.z; nCoord.z += CHUNK_SIZE)
		for (nCoord.x = coord.x - CHUNK_SIZE; nCoord.x <= max.x; nCoord.x += CHUNK_SIZE)
			for (nCoord.y = coord.y - CHUNK_SIZE; nCoord.y <= max.y; nCoord.y += CHUNK_SIZE)
			{
				if (nCoord.y < 0 || nCoord.y >= CHUNK_MAX_HEIGHT || (coord.x == nCoord.x && coord.y == nCoord.y && coord.z == nCoord.z)
					|| ExistChunk(nCoord))
					continue;

				if (firstMiss)
				{
					firstMiss = false;
					chunk->SetStage(Chunk::Stage::WaitingNeighbors);
				}

				BuildChunk(nCoord, false);
				allNeighbors = false;
			}

	return allNeighbors;
}

bool CubeWorld::GetChunkNeighbors(Chunk* chunk, const glm::vec3& coord, Chunk* chunks[26])
{
	const glm::vec3 max{ coord.x + CHUNK_SIZE, coord.y + CHUNK_SIZE, coord.z + CHUNK_SIZE };

	glm::vec3 nCoord{};

	bool allNeighbors = true, firstMiss = true;

	int i = -1;
	for (nCoord.z = coord.z - CHUNK_SIZE; nCoord.z <= max.z; nCoord.z += CHUNK_SIZE)
		for (nCoord.x = coord.x - CHUNK_SIZE; nCoord.x <= max.x; nCoord.x += CHUNK_SIZE)
			for (nCoord.y = coord.y - CHUNK_SIZE; nCoord.y <= max.y; nCoord.y += CHUNK_SIZE)
			{
				++i;
				if (nCoord.y < 0 || nCoord.y > CHUNK_MAX_HEIGHT || (coord.x == nCoord.x && coord.y == nCoord.y && coord.z == nCoord.z)
					|| GetChunk(nCoord, &chunks[i]))
					continue;

				if (firstMiss)
				{
					firstMiss = false;
					chunk->SetStage(Chunk::Stage::WaitingNeighbors);
				}

				BuildChunk(nCoord, false);
				allNeighbors = false;
			}

	return allNeighbors;
}

bool CubeWorld::ExistChunk(const glm::vec3& coord)
{
	std::lock_guard<std::mutex> chunkL(m_ChunksLock);

	return m_Chunks.find(coord) != m_Chunks.end();
}

bool CubeWorld::GetChunk(const glm::vec3& coord, Chunk** chunk)
{
	std::lock_guard<std::mutex> chunkL(m_ChunksLock);

	auto chunkIT = m_Chunks.find(coord);
	if (chunkIT == m_Chunks.end())
		return false;

	*chunk = chunkIT->second;
	return true;
}

void CubeWorld::OnWindowResize()
{
	m_Width  = m_Specification->Width;
	m_Height = m_Specification->Height;

	m_WidthScaled  = (uint32_t)(m_Width  * m_Specification->ScreenScaleFactor);
	m_HeightScaled = (uint32_t)(m_Height * m_Specification->ScreenScaleFactor);

	m_Camera->WindowResize(m_WidthScaled, m_HeightScaled);
	m_Camera->RecalculateView();
}

std::string CubeWorld::BytesToText(double bytes)
{
	if (bytes < 1000) return FormatFloat(bytes, 2) + "B";
	bytes *= 0.001;
	if (bytes < 1000) return FormatFloat(bytes, 2) + "kB";
	bytes *= 0.001;
	if (bytes < 1000) return FormatFloat(bytes, 2) + "MB";
	bytes *= 0.001;
	if (bytes < 1000) return FormatFloat(bytes, 2) + "GB";
	return std::string();
}

std::string CubeWorld::FormatFloat(double n, int digit)
{
	std::string s = std::to_string(n);
	return s.substr(0, min(s.find(".") + digit, s.size()));
}
