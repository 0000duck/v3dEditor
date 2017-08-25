#pragma once

#include "AABB.h"

namespace ve {

	class Sphere
	{
	public:
		glm::vec3 center;
		float radius;

	public:
		Sphere(void);
		Sphere(const glm::vec3& center, float radius);

		bool Contains(const AABB& aabb) const;
	};

}
