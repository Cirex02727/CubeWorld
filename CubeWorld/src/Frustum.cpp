#include "Frustum.h"

void Frustum::Update(Camera* camera)
{
	const glm::mat4 VP = camera->GetViewProjection();

	m_Planes[0].normal.x = VP[0][3] + VP[0][0];
	m_Planes[0].normal.y = VP[1][3] + VP[1][0];
	m_Planes[0].normal.z = VP[2][3] + VP[2][0];
	m_Planes[0].distance = VP[3][3] + VP[3][0];

	m_Planes[1].normal.x = VP[0][3] - VP[0][0];
	m_Planes[1].normal.y = VP[1][3] - VP[1][0];
	m_Planes[1].normal.z = VP[2][3] - VP[2][0];
	m_Planes[1].distance = VP[3][3] - VP[3][0];

	m_Planes[2].normal.x = VP[0][3] + VP[0][1];
	m_Planes[2].normal.y = VP[1][3] + VP[1][1];
	m_Planes[2].normal.z = VP[2][3] + VP[2][1];
	m_Planes[2].distance = VP[3][3] + VP[3][1];

	m_Planes[3].normal.x = VP[0][3] - VP[0][1];
	m_Planes[3].normal.y = VP[1][3] - VP[1][1];
	m_Planes[3].normal.z = VP[2][3] - VP[2][1];
	m_Planes[3].distance = VP[3][3] - VP[3][1];

	m_Planes[4].normal.x = VP[0][3] + VP[0][2];
	m_Planes[4].normal.y = VP[1][3] + VP[1][2];
	m_Planes[4].normal.z = VP[2][3] + VP[2][2];
	m_Planes[4].distance = VP[3][3] + VP[3][2];

	m_Planes[5].normal.x = VP[0][3] - VP[0][2];
	m_Planes[5].normal.y = VP[1][3] - VP[1][2];
	m_Planes[5].normal.z = VP[2][3] - VP[2][2];
	m_Planes[5].distance = VP[3][3] - VP[3][2];

	for (int i = 0; i < 6; i++)
		m_Planes[i].normalize();
}

bool Frustum::SphereIntersect(const glm::vec3& center, float radius)
{
	for (const Plane& p : m_Planes)
	{
		float distance = glm::dot(p.normal, center) + p.distance;

		if (distance < -radius)
			return false;
	}

	return true;
}
