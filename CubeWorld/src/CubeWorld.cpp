#include "CubeWorld.h"

#include <imgui/imgui.h>
#include <iostream>

void CubeWorld::Init()
{
}

void CubeWorld::Update(float timestep)
{
}

void CubeWorld::Render()
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
}
