#include "CubeWorld.h"

#include "Core.h"

#include "utils/Timer.h"
#include "utils/Benchmark.h"

#include "Application.h"

void CubeWorld::Init()
{
	SettupOpenGLSettings();

	InitFramebuffer();
	InitCrosshair();
	InitInteract();

	m_ThreadPool = std::make_unique<ThreadPool>(std::thread::hardware_concurrency() - 1);
	
	m_Noise = std::make_unique<SimplexNoise>(m_GenerationSettings.Frequency, m_GenerationSettings.Amplitude, m_GenerationSettings.Lacunarity, m_GenerationSettings.Persistence);

	m_Camera = std::make_unique<Camera>(60.0f, 0.05f, 3000.0f);
	m_Camera->SetPosition({ -35.6f, 49.9f, 29.3f });
	m_Camera->SetDirection({ 0.6f, -0.4f, 0.7f });
	m_Camera->OnResize(m_WidthScaled, m_HeightScaled);
	m_Camera->RecalculateView();

	m_Texture = std::make_unique<Texture>("res/textures/terrain.png", GL_NEAREST, GL_NEAREST, true, true, true);

	m_Shader = std::make_unique<Shader>("res/shaders/terrain.shader");
	m_Shader->Bind();
	m_Shader->SetUniform1i("u_Texture", 0);

	{
		ScopedTimer t("Chunk!");

		const glm::vec3 pos{ 0.0f, 0.0f, 0.0f };
		
		Chunk*  chunk = new Chunk{ pos };
		chunk->Fill(m_Noise.get());

		Mesh m;
		chunk->GenerateMesh(m);
		/*Benchmark::Bench(1'000, [&chunk, &m]() {
			chunk->GenerateMesh(m);
		}, [&m]() { m.vertices.clear(); m.indices.clear(); });*/

		chunk->UploadMesh(m);

		m_Chunks[pos] = chunk;
	}

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

	bool VSync = true;
	GLCall(glfwSwapInterval(VSync));
}

void CubeWorld::InitFramebuffer()
{
	OnResize();

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
	m_Camera->OnUpdate(timestep);
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

	for (const auto& [_, chunk] : m_Chunks)
		chunk->Render(m_Shader.get());
}

void CubeWorld::ImGuiRender()
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

	const glm::vec3& camPos = m_Camera->GetPosition();
	ImGui::Text("Camera Position: %.1f, %.1f, %.1f", camPos.x, camPos.y, camPos.z);
	const glm::vec3& camDir = m_Camera->GetDirection();
	ImGui::Text("Camera Direction: %.1f, %.1f, %.1f", camDir.x, camDir.y, camDir.z);

	ImGui::Checkbox("Debug Normal: ", &m_DebugNormal);
	if (m_DebugNormal) m_DebugUV = false;
	ImGui::Checkbox("Debug UV: ", &m_DebugUV);
	if (m_DebugUV) m_DebugNormal = false;
}

void CubeWorld::OnResize()
{
	m_Width  = m_Specification->Width;
	m_Height = m_Specification->Height;

	m_WidthScaled  = (uint32_t)(m_Width  * m_Specification->ScreenScaleFactor);
	m_HeightScaled = (uint32_t)(m_Height * m_Specification->ScreenScaleFactor);
}
