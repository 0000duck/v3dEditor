#pragma once

#include "NodeAttribute.h"
#include "IMaterialContainer.h"
#include "AABB.h"
#include "Sphere.h"

namespace ve {

	class Material;

	class IMesh : public NodeAttribute, public IMaterialContainer
	{
	public:
		virtual ModelPtr GetModel() = 0;

		virtual uint32_t GetPolygonCount() const = 0;
		virtual uint32_t GetBoneCount() const = 0;

		virtual const AABB& GetAABB() const = 0;

		virtual bool GetVisible() const = 0;
		virtual void SetVisible(bool enable) = 0;

		virtual bool GetCastShadow() const = 0;
		virtual void SetCastShadow(bool enable) = 0;

	protected:
		virtual ~IMesh() {}

		virtual void UpdatePipeline(Material* pMaterial, uint32_t subsetIndex) = 0;

		friend class Material;
	};

}