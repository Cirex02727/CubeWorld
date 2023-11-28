#include "Application.h"

#include <stdio.h>
#include <stdlib.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_OPENGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "Core.h"
#include "utils/Instrumentor.h"

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool Application::Init()
{
	// Setup GLFW window
	if (!glfwInit())
		return false;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	m_WindowHandle = glfwCreateWindow(m_Specification.Width, m_Specification.Height, m_Specification.Name.c_str(), NULL, NULL);
	if (!m_WindowHandle)
	{
		glfwTerminate();
		return false;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(m_WindowHandle);

	if (glewInit() != GLEW_OK)
		std::cout << "Error!" << std::endl;

	glfwSetErrorCallback(glfw_error_callback);

	PROFILE_BEGIN_SESSION("Application", "result.json");

	GLCall(std::cout << glGetString(GL_VERSION) << std::endl);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(m_WindowHandle, true);
	ImGui_ImplOpenGL3_Init("#version 430");

	m_World = new CubeWorld();

	return true;
}

void Application::Run()
{
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ImGuiIO& io = ImGui::GetIO();

	// Main loop
	while (!glfwWindowShouldClose(m_WindowHandle))
	{
		glfwPollEvents();

		// Update
		m_World->Update(m_TimeStep);

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Render
		GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_World->Render();

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(m_WindowHandle);

		float time = (float)glfwGetTime();
		m_FrameTime = time - m_LastFrameTime;
		m_TimeStep = glm::min<float>(m_FrameTime, 0.0333f);
		m_LastFrameTime = time;
	}
}

void Application::Shutdown()
{
	delete m_World;

	PROFILE_END_SESSION();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(m_WindowHandle);
	glfwTerminate();
}
