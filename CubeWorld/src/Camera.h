#pragma once

#include "utils/input/Input.h"

#include <glm/glm.hpp>
#include <vector>

class Camera
{
public:
	Camera(float verticalFOV, float nearClip, float farClip);

	bool OnUpdate(float ts);
	void WindowResize(uint32_t width, uint32_t height);

	const glm::mat4& GetProjection() const { return m_Projection; }
	const glm::mat4& GetInverseProjection() const { return m_InverseProjection; }
	const glm::mat4& GetView() const { return m_View; }
	const glm::mat4& GetInverseView() const { return m_InverseView; }

	const glm::mat4 GetViewProjection() const { return m_Projection * m_View; }
	const glm::mat4 GetProjectionOrtho() const { return m_ProjectionOrtho; }

	const glm::vec3& GetPosition() const { return m_Position; }
	const glm::vec3& GetDirection() const { return m_ForwardDirection; }

	void SetPosition(const glm::vec3& position) { m_Position = position; }
	void SetDirection(const glm::vec3& direction) { m_ForwardDirection = direction; }

	float GetRotationSpeed();

	void RecalculateView();

private:
	bool Move(glm::vec3* movement) const;
	bool Rotate();

	void RecalculateProjection();

private:
	glm::mat4 m_Projection{ 1.0f };
	glm::mat4 m_ProjectionOrtho{ 1.0f };
	glm::mat4 m_View{ 1.0f };
	glm::mat4 m_InverseProjection{ 1.0f };
	glm::mat4 m_InverseView{ 1.0f };

	float m_VerticalFOV = 45.0f;
	float m_NearClip = 0.1f;
	float m_FarClip = 100.0f;

	glm::vec3 m_Position{ 0.0f };
	glm::vec3 m_ForwardDirection{ 0.0f };

	glm::vec2 m_LastMousePosition{ 0.0f };

	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

	glm::vec3 m_Velocity, m_Acceleration;

	bool m_PrevWindowFocus = false;
};