#include "Sphere.h"
#include "AABB.h"

namespace ve {

	Sphere::Sphere() :
		radius(0.0f)
	{
	}

	Sphere::Sphere(const glm::vec3& center, float radius)
	{
		this->center = center;
		this->radius = radius;
	}

	bool Sphere::Contains(const AABB& aabb) const
	{
		float elm;
		float tmp;
		glm::vec3 pos;

		for (uint32_t i = 0; i < 3; i++)
		{
			elm = center[i];

			tmp = aabb.minimum[i];
			if (elm < tmp) { elm = tmp; }

			tmp = aabb.maximum[i];
			if (elm > tmp) { elm = tmp; }

			pos[i] = elm;
		}

		glm::vec3 vec = pos - center;

		return (glm::length2(vec) <= (radius * radius));
	}

}
