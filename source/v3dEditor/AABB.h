#pragma once

namespace ve {

	class AABB
	{
	public:
		glm::vec3 minimum;
		glm::vec3 maximum;
		glm::vec3 center;
		glm::vec3 points[8];

		AABB(void);
		AABB(const glm::vec3& minimum, const glm::vec3& maximum);

		AABB& UpdateMinMax();
		AABB& UpdateCenterAndPoints();
	};

}
