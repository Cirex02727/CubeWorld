#pragma once

#include "Layer.h"

#include "CubeWorld.h"

#include <string>

struct GLFWwindow;

struct ApplicationSpecification
{
	std::string Name = "Base App";
	uint32_t Width = 960;
	uint32_t Height = 540;
};

class Application
{
public:
	Application(const ApplicationSpecification& applicationSpecification = ApplicationSpecification())
		: m_Specification(applicationSpecification) {}
	~Application() {}

	bool Init();

	void Run();

	void Shutdown();

private:
	ApplicationSpecification m_Specification;
	GLFWwindow* m_WindowHandle = nullptr;

	CubeWorld* m_World;

	float m_TimeStep = 0.0f;
	float m_FrameTime = 0.0f;
	float m_LastFrameTime = 0.0f;
};
