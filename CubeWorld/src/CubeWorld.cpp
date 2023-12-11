#include "CubeWorld.h"

#include "Core.h"

#include "utils/Timer.h"
#include "utils/Benchmark.h"

#include "data/BlocksManager.h"

#include "Application.h"

void CubeWorld::Init()
{
	SettupOpenGLSettings();

	InitFramebuffer();
	InitCrosshair();
	InitInteract();

	m_ThreadPool = std::make_unique<ThreadPool>(std::thread::hardware_concurrency() - 1);
	
	m_Noise = std::make_unique<SimplexNoise>(m_GenerationSettings.Frequency, m_GenerationSettings.Amplitude, m_GenerationSettings.Lacunarity, m_GenerationSettings.Persistence);

	BlocksManager::Init();

	m_Camera = std::make_unique<Camera>(60.0f, 0.05f, 3000.0f);
	m_Camera->SetPosition({ 0.0f, CHUNK_MAX_MOUNTAIN, 0.0f });
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


	// Blocks Manager
	{
		float stepX = 1.0f / (m_Texture->GetWidth() / 16.0f);
		float stepY = 1.0f / (m_Texture->GetHeight() / 16.0f);
		m_AtlasStep = glm::vec2{ stepX, stepY };

		m_Shader->Bind();
		m_Shader->SetUniform2f("u_Step", m_AtlasStep.x, m_AtlasStep.y);

		Block* block;
		block = new Block("Air",     { { 10 * stepX, 14 * stepY } });
		block->m_IsTransparent = true;

		block = new Block("Dirt",    { {  2 * stepX, 15 * stepY } });
		block = new Block("Grass",   { {  0 * stepX, 15 * stepY } });
		block = new Block("Stone",   { {  1 * stepX, 15 * stepY } });

		block = new Block("Water",   { { 13 * stepX,  3 * stepY } });
		block->m_IsTransparent = true;
		block->m_IsTranslucent = true;
		block->m_IsLiquid      = true;

		block = new Block("Glass",   { {  1 * stepX, 12 * stepY } });
		block->m_IsTransparent = true;

		block = new Block("Sand",    { {  2 * stepX, 14 * stepY } });
		block = new Block("Snow",    { {  2 * stepX, 11 * stepY } });

		block = new Block("Bedrock", { {  1 * stepX, 14 * stepY } });

		BlocksManager::UploadBlocks();
	}

	// To Implement
	// StructureManager (Load Structures)
	// DataCompressManager
	// Custome Blocks (ShaderCustome)
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
	BlocksManager::Dispose();

	GLCall(glDeleteVertexArrays(1, &m_CrosshairVAO));
	GLCall(glDeleteBuffers(1, &m_CrosshairVBO));

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

	const glm::vec3 cameraPosition = m_Camera->GetPosition();
	const glm::vec3 cameraChunk = glm::floor(cameraPosition * CHUNK_SIZE_INV);

	{
		int renderDist = m_Settings.RenderDistance, maxRenderDist = m_Settings.MaxRenderDistance;

		if (m_GeneratingChunks.size() < 100 && std::abs(maxRenderDist - renderDist) > 0)
		{
			m_Settings.RenderDistance += glm::sign(maxRenderDist - renderDist);

			renderDist = m_Settings.RenderDistance;

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
			size_t tvertices = mesh.tvertices.size();
			if (vertices <= 0 && tvertices <= 0)
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
				m_TotalBytes += vertices * sizeof(uint32_t) + tvertices * sizeof(uint32_t);
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


	if (Input::IsKeyDown(KeyCode::E))
		PlaceBlock(cameraPosition, BlocksManager::GetBlock("Dirt"), FaceSide::Front);

	if (Input::IsKeyDown(KeyCode::Q))
		PlaceBlock(cameraPosition, BlocksManager::GetBlock("Air"), FaceSide::Front);
}

void CubeWorld::Render()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glViewport(0, 0, m_WidthScaled, m_HeightScaled);

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

	BlocksManager::Bind();

	m_RenderedChunk = 0;

	{
		std::lock_guard<std::mutex> chunksL(m_MeshedChunksLock);

		std::vector<std::tuple<glm::vec3, Chunk*>> inFrustum;
		inFrustum.reserve(m_MeshedChunks.size());

		Shader* shader = m_Shader.get();
		for (auto& [coord, chunk] : m_MeshedChunks)
		{
			if (!m_Frustum->SphereIntersect(coord + HCHUNK_SIZE, SPHERE_CHUNK_RADIUS))
				continue;

			chunk->Render(shader);
			inFrustum.push_back({ coord, chunk });
			++m_RenderedChunk;
		}

		// Order Chunks based on distance from camera
		std::sort(inFrustum.begin(), inFrustum.end(), [&camPos](const std::tuple<glm::vec3, Chunk*>& c1, const std::tuple<glm::vec3, Chunk*>& c2)
		{
			return glm::distance(std::get<0>(c1) + HCHUNK_SIZE, camPos) > glm::distance(std::get<0>(c2) + HCHUNK_SIZE, camPos);
		});

		// Render Transparence Faces
		GLCall(glEnable(GL_BLEND));
		GLCall(glDisable(GL_CULL_FACE));

		GLCall(glDepthMask(GL_FALSE));

		for (const auto& [_, chunk] : inFrustum)
		{
			chunk->RenderT(shader);
		}

		GLCall(glDepthMask(GL_TRUE));
	}

	// Render 2D Objects (UI)

	{
		// Render 2D Objects (UI)
		m_CrosshairShader->Bind();
		m_CrosshairShader->SetUniformMat4f("u_VP", m_Camera->GetProjectionOrtho());

		m_CrosshairTexture->Bind();

		glBindVertexArray(m_CrosshairVAO);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	{
		// Draw Framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
		GLCall(glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT));
		glViewport(0, 0, m_Width, m_Height);

		m_FBOShader->Bind();
		glBindVertexArray(m_FBOQuadVAO);

		glDisable(GL_DEPTH_TEST);

		GLCall(glActiveTexture(GL_TEXTURE0));
		GLCall(glBindTexture(GL_TEXTURE_2D, m_FBOColor));
		glGenerateMipmap(GL_TEXTURE_2D);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glEnable(GL_DEPTH_TEST);
	}

	GLCall(glDisable(GL_BLEND));
	glEnable(GL_CULL_FACE);
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
		for (nCoord.y = coord.y - CHUNK_SIZE; nCoord.y <= max.y; nCoord.y += CHUNK_SIZE)
			for (nCoord.x = coord.x - CHUNK_SIZE; nCoord.x <= max.x; nCoord.x += CHUNK_SIZE)
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

	if (mesh.vertices.size() > 0 || mesh.tvertices.size() > 0)
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
		for (nCoord.y = coord.y - CHUNK_SIZE; nCoord.y <= max.y; nCoord.y += CHUNK_SIZE)
			for (nCoord.x = coord.x - CHUNK_SIZE; nCoord.x <= max.x; nCoord.x += CHUNK_SIZE)
			{
				if (nCoord.y < 0 || nCoord.y > CHUNK_MAX_HEIGHT || (coord.x == nCoord.x && coord.y == nCoord.y && coord.z == nCoord.z)
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
		for (nCoord.y = coord.y - CHUNK_SIZE; nCoord.y <= max.y; nCoord.y += CHUNK_SIZE)
			for (nCoord.x = coord.x - CHUNK_SIZE; nCoord.x <= max.x; nCoord.x += CHUNK_SIZE)
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

void CubeWorld::PlaceBlock(Chunk* chunk, glm::vec3 coord, int data, FaceSide side)
{
	chunk->PlaceBlock((uint32_t)coord.x, (uint32_t)coord.y, (uint32_t)coord.z, data, (Block::Side)side);

	const glm::vec3& chunkCoord = chunk->m_Coord;
	SetChunkDirty(chunk, chunkCoord);

	if (coord.x == 0)
		SetChunkDirty(chunk, glm::vec3{ chunkCoord.x - CHUNK_SIZE, chunkCoord.y, chunkCoord.z });
	if (coord.x == CHUNK_SIZE - 1)
		SetChunkDirty(chunk, glm::vec3{ chunkCoord.x + CHUNK_SIZE, chunkCoord.y, chunkCoord.z });
	if (coord.y == 0 && chunk->m_Coord.y > 0)
		SetChunkDirty(chunk, glm::vec3{ chunkCoord.x, chunkCoord.y - CHUNK_SIZE, chunkCoord.z });
	if (coord.y == CHUNK_SIZE - 1)
		SetChunkDirty(chunk, glm::vec3{ chunkCoord.x, chunkCoord.y + CHUNK_SIZE, chunkCoord.z });
	if (coord.z == 0)
		SetChunkDirty(chunk, glm::vec3{ chunkCoord.x, chunkCoord.y, chunkCoord.z - CHUNK_SIZE });
	if (coord.z == CHUNK_SIZE - 1)
		SetChunkDirty(chunk, glm::vec3{ chunkCoord.x, chunkCoord.y, chunkCoord.z + CHUNK_SIZE });
}

void CubeWorld::PlaceBlock(glm::vec3 coord, Block* block, FaceSide side)
{
	if (!block->CanBePlaced(this, (int)std::floor(coord.x), (int)std::floor(coord.y), (int)std::floor(coord.z)))
		return;

	const glm::vec3 chunkCoord = WorldToChunkPos(coord);

	Chunk* chunk;
	if (GetChunk(chunkCoord, &chunk))
		PlaceBlock(chunk, coord, block->m_ID, side);
}

glm::vec3 CubeWorld::WorldToChunkPos(glm::vec3& worldPos)
{
	const glm::vec3 chunkCoord = glm::floor(worldPos * CHUNK_SIZE_INV) * CHUNK_SIZE3;

	worldPos = { (int)std::floor(worldPos.x) % CHUNK_SIZE, (int)std::floor(worldPos.y) % CHUNK_SIZE, (int)std::floor(worldPos.z) % CHUNK_SIZE };

	if (worldPos.x < 0) worldPos.x += CHUNK_SIZE;
	if (worldPos.y < 0) worldPos.y += CHUNK_SIZE;
	if (worldPos.z < 0) worldPos.z += CHUNK_SIZE;

	return chunkCoord;
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
