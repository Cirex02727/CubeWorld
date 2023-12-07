#pragma once

#include "Layer.h"

#include "CubeWorld.h"

#include <string>

struct GLFWwindow;

struct WindowSpecification
{
	uint32_t Width = 960;
	uint32_t Height = 540;

	float ScreenScaleFactor = 3;

	int MaxFps = 120;
	float MaxDeltaTime = 1.0f / MaxFps;
};

class Application
{
public:
	Application() {}
	~Application() {}

	bool Init();

	void Run();

	void Shutdown();

	void OnWindowResize(int width, int height);

private:
	WindowSpecification m_Specification;
	GLFWwindow* m_WindowHandle = nullptr;

	CubeWorld* m_World = nullptr;
	
	float m_DeltaTime = 0.0f;
	float m_LastFrameTime = 0.0f;
};
