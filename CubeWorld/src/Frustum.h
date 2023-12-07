#pragma once

#include "Camera.h"

#include <glm/glm.hpp>

struct Plane
{
public:
	glm::vec3 normal;
	float distance;

	void normalize()
	{
		float mag = glm::length(normal);
		normal /= mag;
		distance /= mag;
	}
};

class Frustum
{
public:
	Frustum() = default;
	~Frustum() = default;

	void Update(Camera* camera);

	bool SphereIntersect(const glm::vec3& center, float radius);

private:
	Plane m_Planes[6]{};
};