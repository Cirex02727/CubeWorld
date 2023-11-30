#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

Camera::Camera(float verticalFOV, float nearClip, float farClip)
	: m_Position(), m_ForwardDirection({ 0.0f, 0.0f, 1.0f }), m_VerticalFOV(verticalFOV), m_NearClip(nearClip), m_FarClip(farClip)
{
	RecalculateView();
}

bool Camera::OnUpdate(float ts)
{
	if (!Input::IsMouseButtonDown(MouseButton::Right))
	{
		if (m_PrevWindowFocus)
		{
			Input::SetCursorMode(CursorMode::Normal);
			m_PrevWindowFocus = false;
		}

		return false;
	}
	else if (Input::IsMouseButtonDown(MouseButton::Right) && !m_PrevWindowFocus)
	{
		Input::SetCursorMode(CursorMode::Locked);
		m_PrevWindowFocus = true;

		m_LastMousePosition = Input::GetMousePosition();
	}

	glm::vec3 movement{};
	bool moved = Move(&movement);

	movement *= 75.0f;

	if (Input::IsKey(KeyCode::LeftControl))
		movement *= 10;

	// m_Acceleration = moved ? movement : glm::vec3{};
	// m_Velocity += m_Acceleration * ts;
	// m_Velocity *= 0.9;

	m_Position += movement * ts;

	moved |= Rotate();

	if (moved)
		RecalculateView();

	return moved;
}

void Camera::OnResize(uint32_t width, uint32_t height)
{
	if (width == m_ViewportWidth && height == m_ViewportHeight)
		return;

	m_ViewportWidth = width;
	m_ViewportHeight = height;

	m_ProjectionOrtho = glm::ortho(0.0f, (float)m_ViewportWidth, 0.0f, (float)m_ViewportHeight, -1.0f, 1.0f);

	RecalculateProjection();
}

float Camera::GetRotationSpeed()
{
	return 0.001f;
}

bool Camera::Move(glm::vec3* movement)
{
	bool moved = false;

	constexpr glm::vec3 upDirection(0.0f, 1.0f, 0.0f);
	const glm::vec3 rightDirection = glm::cross(m_ForwardDirection, upDirection);

	const glm::vec3 forward = glm::normalize(glm::vec3{ m_ForwardDirection.x, 0, m_ForwardDirection.z });
	const glm::vec3 right = glm::normalize(glm::vec3{ rightDirection.x, 0, rightDirection.z });
	if (Input::IsKey(KeyCode::W))
	{
		*movement += forward;
		moved = true;
	}
	else if (Input::IsKey(KeyCode::S))
	{
		*movement -= forward;
		moved = true;
	}
	if (Input::IsKey(KeyCode::A))
	{
		*movement -= right;
		moved = true;
	}
	else if (Input::IsKey(KeyCode::D))
	{
		*movement += right;
		moved = true;
	}

	if (Input::IsKey(KeyCode::LeftShift))
	{
		*movement -= upDirection;
		moved = true;
	}
	else if (Input::IsKey(KeyCode::Space))
	{
		*movement += upDirection;
		moved = true;
	}

	return moved;
}

bool Camera::Rotate()
{
	glm::vec2 mousePos = Input::GetMousePosition();
	glm::vec2 delta = mousePos - m_LastMousePosition;
	m_LastMousePosition = mousePos;

	constexpr glm::vec3 upDirection(0.0f, 1.0f, 0.0f);
	glm::vec3 rightDirection = glm::cross(m_ForwardDirection, upDirection);

	if (delta.x == 0.0f && delta.y == 0.0f)
		return false;

	float pitchDelta = delta.y * GetRotationSpeed();
	float yawDelta = delta.x * GetRotationSpeed();

	glm::quat q = glm::normalize(glm::cross(glm::angleAxis(-pitchDelta, rightDirection),
		glm::angleAxis(-yawDelta, glm::vec3(0.f, 1.0f, 0.0f))));
	m_ForwardDirection = glm::rotate(q, m_ForwardDirection);

	return true;
}

void Camera::RecalculateProjection()
{
	m_Projection = glm::perspective(glm::radians(m_VerticalFOV), (float)m_ViewportWidth / (float)m_ViewportHeight, m_NearClip, m_FarClip);
	m_InverseProjection = glm::inverse(m_Projection);
}

void Camera::RecalculateView()
{
	m_View = glm::lookAt(m_Position, m_Position + m_ForwardDirection, glm::vec3(0, 1, 0));
	m_InverseView = glm::inverse(m_View);
}
