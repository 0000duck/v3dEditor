#pragma once

#include "Frustum.h"
#include "DynamicContainer.h"
#include "Sphere.h"
#include "AABB.h"

namespace ve {

	class DebugRenderer;

	class NodeAttribute
	{
	public:
		enum TYPE
		{
			TYPE_UNDEFINED = 0,
			TYPE_EMPTY = 1,
			TYPE_CAMERA = 2,
			TYPE_LIGHT = 3,
			TYPE_MODEL = 4,
			TYPE_MESH = 5,
		};

		NodeAttribute();
		virtual ~NodeAttribute();

		NodePtr GetOwner();
		virtual NodeAttribute::TYPE GetType() const = 0;

		VE_DECLARE_ALLOCATOR

	protected:
		virtual void Update(const glm::mat4& worldMatrix) {};

		virtual bool Draw(
			const Frustum& frustum,
			uint32_t frameIndex,
			collection::DynamicContainer<OpacityDrawSet>& opacityDrawSets,
			collection::DynamicContainer<TransparencyDrawSet>& transparencyDrawSets) { return true; }

		virtual void DrawShadow(
			const Sphere& sphere,
			uint32_t frameIndex,
			collection::DynamicContainer<ShadowDrawSet>& shadowDrawSets,
			AABB& aabb) {}

		virtual void DebugDraw(uint32_t flags, DebugRenderer* pDebugRenderer) {}

		virtual bool IsSelectSupported() const { return false; }
		virtual void SetSelectKey(uint32_t key) {}
		virtual void DrawSelect(uint32_t frameIndex, SelectDrawSet& drawSet) {}

	private:
		struct Impl;
		Impl* impl;

		void SetOwnerNode(NodePtr node);

		friend class Node;
		friend class NodeSelector;
		friend class Scene;
	};

}