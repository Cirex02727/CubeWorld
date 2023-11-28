#pragma once

#include "Core.h"

#include "KeyCodes.h"

#include <glm/glm.hpp>

class Input
{
public:
	static void Init(GLFWwindow* window);

	static bool IsKey(KeyCode keycode);
	static bool IsKeyDown(KeyCode keycode);
	static bool IsKeyUp(KeyCode keycode);

	static bool IsMouseButtonDown(MouseButton button);

	static glm::vec2 GetMousePosition();

	static void SetCursorMode(CursorMode mode);


	static GLFWwindow* m_Window;

	static bool m_PrevKeys[(int)KeyCode::Count];
};
