#include "Frustum.h"

namespace ve {

	void Frustum::Update(const glm::mat4 &viewProjMatrix)
	{
		Plane* plane;

		// NearPlane
		plane = &m_Planes[Frustum::PLANE_TYPE_NEAR];
		plane->normal.x = viewProjMatrix[0].w + viewProjMatrix[0].z;
		plane->normal.y = viewProjMatrix[1].w + viewProjMatrix[1].z;
		plane->normal.z = viewProjMatrix[2].w + viewProjMatrix[2].z;
		plane->d = viewProjMatrix[3].w + viewProjMatrix[3].z;
		plane->Normalize();

		// FarPlane
		plane = &m_Planes[Frustum::PLANE_TYPE_FAR];
		plane->normal.x = viewProjMatrix[0].w - viewProjMatrix[0].z;
		plane->normal.y = viewProjMatrix[1].w - viewProjMatrix[1].z;
		plane->normal.z = viewProjMatrix[2].w - viewProjMatrix[2].z;
		plane->d = viewProjMatrix[3].w - viewProjMatrix[3].z;
		plane->Normalize();

		// TopPlane
		plane = &m_Planes[Frustum::PLANE_TYPE_TOP];
		plane->normal.x = viewProjMatrix[0].w + viewProjMatrix[0].y;
		plane->normal.y = viewProjMatrix[1].w + viewProjMatrix[1].y;
		plane->normal.z = viewProjMatrix[2].w + viewProjMatrix[2].y;
		plane->d = viewProjMatrix[3].w + viewProjMatrix[3].y;
		plane->Normalize();

		// BottomPlane
		plane = &m_Planes[Frustum::PLANE_TYPE_BOTTOM];
		plane->normal.x = viewProjMatrix[0].w - viewProjMatrix[0].y;
		plane->normal.y = viewProjMatrix[1].w - viewProjMatrix[1].y;
		plane->normal.z = viewProjMatrix[2].w - viewProjMatrix[2].y;
		plane->d = viewProjMatrix[3].w - viewProjMatrix[3].y;
		plane->Normalize();

		// LeftPlane
		plane = &m_Planes[Frustum::PLANE_TYPE_LEFT];
		plane->normal.x = viewProjMatrix[0].w + viewProjMatrix[0].x;
		plane->normal.y = viewProjMatrix[1].w + viewProjMatrix[1].x;
		plane->normal.z = viewProjMatrix[2].w + viewProjMatrix[2].x;
		plane->d = viewProjMatrix[3].w + viewProjMatrix[3].x;
		plane->Normalize();

		// RightPlane
		plane = &m_Planes[Frustum::PLANE_TYPE_RIGHT];
		plane->normal.x = viewProjMatrix[0].w - viewProjMatrix[0].x;
		plane->normal.y = viewProjMatrix[1].w - viewProjMatrix[1].x;
		plane->normal.z = viewProjMatrix[2].w - viewProjMatrix[2].x;
		plane->d = viewProjMatrix[3].w - viewProjMatrix[3].x;
		plane->Normalize();
	}

	const Plane& Frustum::GetPlane(Frustum::PLANE_TYPE type) const
	{
		return m_Planes[type];
	}

	bool Frustum::Contains(const Sphere& sphere) const
	{
		const glm::vec3& center = sphere.center;
		const float& radius = sphere.radius;

		const Plane* plane = m_Planes;
		const Plane* planeEnd = plane + 6;

		while (plane != planeEnd)
		{
			const glm::vec3& normal = plane->normal;
			float distance = (normal.x * center.x) + (normal.y * center.y) + (normal.z * center.z) + plane->d;

			if (-radius > distance)
			{
				return false;
			}

			plane++;
		}

		return true;
	}

	bool Frustum::Contains(const AABB& aabb) const
	{
		const Plane* pPlane = &(m_Planes[0]);
		const Plane* pPlaneEnd = pPlane + 6;

		const glm::vec3* pPointBegin = &(aabb.points[0]);
		const glm::vec3* pPointEnd = pPointBegin + 8;
		const glm::vec3* pPoint = NULL;

		float dist;
		uint32_t count;

		while (pPlane != pPlaneEnd)
		{
			const glm::vec3& norm = pPlane->normal;

			count = 8;
			pPoint = pPointBegin;
			while (pPoint != pPointEnd)
			{
				dist = (norm.x * pPoint->x) + (norm.y * pPoint->y) + (norm.z * pPoint->z) + pPlane->d;
				if (dist < 0.0f)
				{
					VE_ASSERT(count > 0);
					count--;
				}

				pPoint++;
			}

			if (count == 0)
			{
				return false;
			}

			pPlane++;
		}

		return true;
	}

}
