#pragma once

#include "Frustum.h"
#include "DynamicContainer.h"

namespace ve {

	class NodeAttribute;
	class DebugRenderer;

	class Node
	{
	public:
		static NodePtr Create(int32_t id = -1);

		Node();
		virtual ~Node();

		int32_t GetID() const;

		const char* GetNameA() const;
		const wchar_t* GetName() const;
		void SetName(const wchar_t* pName);

		uint32_t GetGroupFlags() const;
		void SetGroupFlags(uint32_t flags);

		NodePtr GetParent();
		uint32_t GetChildCount() const;
		NodePtr GetChild(uint32_t childIndex);
		static NodePtr AddChild(NodePtr parent, int32_t id = -1);
		static void AddChild(NodePtr parent, NodePtr child);
		NodePtr FindChild(const wchar_t* pName);
		static NodePtr Find(NodePtr node, const wchar_t* pName);
		static void RemoveByName(NodePtr node, const wchar_t* pName);
		static void RemoveByGroup(NodePtr node, uint32_t groupMask);

		const glm::mat4& GetLocalMatrix() const;
		const Transform& GetLocalTransform() const;
		void SetLocalTransform(const Transform& transform);
		void SetLocalTransform(const glm::vec3& scale, const glm::quat& rotation, const glm::vec3 translation);

		const glm::mat4& GetWorldMatrix() const;
		const Transform& GetWorldTransform() const;

		NodeAttributePtr GetAttribute();
		static void SetAttribute(NodePtr node, NodeAttributePtr attribute);

		void Update();
		void Update(const glm::mat4& worldMatrix);

		void Draw(
			const Frustum& frustum,
			uint32_t frameIndex,
			collection::DynamicContainer<OpacityDrawSet>& opacityDrawSets,
			collection::DynamicContainer<TransparencyDrawSet>& transparencyDrawSets,
			uint32_t debugFlags, DebugRenderer* pDebugRenderer);

		void DrawShadow(
			const Sphere& sphere,
			uint32_t frameIndex,
			collection::DynamicContainer<ShadowDrawSet>& shadowDrawSets,
			AABB& aabb);

		VE_DECLARE_ALLOCATOR

	private:
		struct Impl;
		Impl* impl;
	};

}