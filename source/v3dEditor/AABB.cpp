#include "AABB.h"

namespace ve {

	AABB::AABB() :
		minimum(+FLT_MAX, +FLT_MAX, +FLT_MAX),
		maximum(-FLT_MAX, -FLT_MAX, -FLT_MAX),
		center(0.0f, 0.0f, 0.0f)
	{
	}

	AABB::AABB(const glm::vec3& minimum, const glm::vec3& maximum)
	{
		this->minimum = minimum;
		this->maximum = maximum;
		center = (minimum + maximum) * 0.5f;
	}

	AABB& AABB::UpdateMinMax()
	{
		const glm::vec3* pPoint = &points[0];
		const glm::vec3* pPointEnd = pPoint + 8;

		minimum.x = +VE_FLOAT_MAX;
		minimum.y = +VE_FLOAT_MAX;
		minimum.z = +VE_FLOAT_MAX;

		maximum.x = -VE_FLOAT_MAX;
		maximum.y = -VE_FLOAT_MAX;
		maximum.z = -VE_FLOAT_MAX;

		while (pPoint != pPointEnd)
		{
			minimum = glm::min(minimum, *pPoint);
			maximum = glm::min(maximum, *pPoint);
			pPoint++;
		}

		return *this;
	}

	AABB& AABB::UpdateCenterAndPoints(void)
	{
		//è„ïΩñ 
		points[0].x = minimum.x;
		points[0].y = maximum.y;
		points[0].z = maximum.z;
		points[1].x = maximum.x;
		points[1].y = maximum.y;
		points[1].z = maximum.z;
		points[2].x = maximum.x;
		points[2].y = maximum.y;
		points[2].z = minimum.z;
		points[3].x = minimum.x;
		points[3].y = maximum.y;
		points[3].z = minimum.z;

		//â∫ïΩñ 
		points[4].x = minimum.x;
		points[4].y = minimum.y;
		points[4].z = maximum.z;
		points[5].x = maximum.x;
		points[5].y = minimum.y;
		points[5].z = maximum.z;
		points[6].x = maximum.x;
		points[6].y = minimum.y;
		points[6].z = minimum.z;
		points[7].x = minimum.x;
		points[7].y = minimum.y;
		points[7].z = minimum.z;

		//íÜêS
		center = (minimum + maximum) * 0.5f;

		return *this;
	}

}
