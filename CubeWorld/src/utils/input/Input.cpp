#include "Input.h"

GLFWwindow* Input::m_Window = nullptr;

bool Input::m_PrevKeys[(int)KeyCode::Count];

void Input::Init(GLFWwindow* window)
{
	m_Window = window;
	memset(m_PrevKeys, 0, (int)KeyCode::Count * sizeof(bool));
}

bool Input::IsKey(KeyCode keycode)
{
	int state = glfwGetKey(m_Window, (int)keycode);

	bool pressed = ((state == GLFW_PRESS || state == GLFW_REPEAT) ? true : false);
	m_PrevKeys[(int)keycode] = pressed;

	return pressed;
}

bool Input::IsKeyDown(KeyCode keycode)
{
	bool prevKey = m_PrevKeys[(int)keycode];
	return IsKey(keycode) && !prevKey;
}

bool Input::IsKeyUp(KeyCode keycode)
{
	bool prevKey = m_PrevKeys[(int)keycode];
	return !IsKey(keycode) && prevKey;
}

bool Input::IsMouseButtonDown(MouseButton button)
{
	int state = glfwGetMouseButton(m_Window, (int)button);
	return state == GLFW_PRESS;
}

glm::vec2 Input::GetMousePosition()
{
	double x, y;
	glfwGetCursorPos(m_Window, &x, &y);
	return { (float)x, (float)y };
}

void Input::SetCursorMode(CursorMode mode)
{
	glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);
}
