#pragma once

namespace ve {

	class Plane
	{
	public:
		glm::vec3 normal;
		float d;

		Plane();
		Plane(const glm::vec3& normal, float d);

		void Normalize();

		static Plane Calculate(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3)
		{
			glm::vec3 v1;
			glm::vec3 v2;

			v1 = p2 - p1;
			v2 = p3 - p1;

			Plane plane;
			plane.normal = glm::normalize(glm::cross(v1, v2));

			plane.d = glm::dot(plane.normal, p1);

			return plane;
		}
	};

}
