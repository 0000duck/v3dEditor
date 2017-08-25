#include "Node.h"
#include "NodeAttribute.h"

namespace ve {

	/*******************************/
	/* private - struct Node::Impl */
	/*******************************/

	struct Node::Impl
	{
		class NullAttribute : public NodeAttribute
		{
		public:
			NullAttribute() {}
			virtual ~NullAttribute() {}

			NodeAttribute::TYPE GetType() const override { return NodeAttribute::TYPE_UNDEFINED; }
			void Update(const glm::mat4& worldMatrix) override {}

		protected:
			bool IsSelectSupported() const override { return false; }
			void SetSelectKey(uint32_t key) override {}
			void DrawSelect(uint32_t frameIndex, SelectDrawSet& drawSet) {}

		private:
			WeakPtr<Node> m_Owner;
		};

		int32_t id;

		StringW name;
		StringA nameA;

		uint32_t groupFlags;

		WeakPtr<Node> parent;
		collection::Vector<NodePtr> childs;

		Transform localTransform;
		glm::mat4 localMatrix;

		Transform worldTransform;
		glm::mat4 worldMatrix;

		NodeAttributePtr attribute;
		
		Impl() :
			id(-1),
			groupFlags(0),
			localMatrix(glm::mat4(1.0f)),
			worldMatrix(glm::mat4(1.0f)),
			attribute(std::make_shared<Impl::NullAttribute>())
		{
			localTransform.scale = glm::vec3(1.0f);
			localTransform.rotation = glm::quat();
			localTransform.translation = glm::vec3(0.0f);
		}

		VE_DECLARE_ALLOCATOR
	};

	/*****************/
	/* public - Node */
	/*****************/

	NodePtr Node::Create(int32_t id)
	{
		NodePtr node = std::make_shared<Node>();

		node->impl->id = id;
		Node::SetAttribute(node, node->impl->attribute);

		return std::move(node);
	}

	Node::Node() :
		impl(VE_NEW_T(Node::Impl))
	{
	}

	Node::~Node()
	{
		VE_DELETE_T(impl, Impl);
	}

	int32_t Node::GetID() const
	{
		return impl->id;
	}

	const char* Node::GetNameA() const
	{
		return impl->nameA.c_str();
	}

	const wchar_t* Node::GetName() const
	{
		return impl->name.c_str();
	}

	uint32_t Node::GetGroupFlags() const
	{
		return impl->groupFlags;
	}

	void Node::SetGroupFlags(uint32_t flags)
	{
		impl->groupFlags = flags;
	}

	void Node::SetName(const wchar_t* pName)
	{
		impl->name = pName;
		ToMultibyteString(pName, impl->nameA);
	}

	NodePtr Node::GetParent()
	{
		return impl->parent.lock();
	}

	uint32_t Node::GetChildCount() const
	{
		return static_cast<uint32_t>(impl->childs.size());
	}

	NodePtr Node::GetChild(uint32_t childIndex)
	{
		return impl->childs[childIndex];
	}

	NodePtr Node::AddChild(NodePtr parent, int32_t id)
	{
		NodePtr child = Node::Create(id);
		child->impl->parent = parent;
		parent->impl->childs.push_back(child);
		return child;
	}

	void Node::AddChild(NodePtr parent, NodePtr child)
	{
		parent->impl->childs.push_back(child);
		child->impl->parent = parent;
	}

	NodePtr Node::FindChild(const wchar_t* pName)
	{
		NodePtr ret;

		uint32_t childCount = GetChildCount();
		for (uint32_t i = 0; (i < childCount) && (ret == nullptr); i++)
		{
			ret = Node::Find(GetChild(i), pName);
		}

		return ret;
	}

	NodePtr Node::Find(NodePtr node, const wchar_t* pName)
	{
		NodePtr ret;

		if (wcscmp(node->GetName(), pName) == 0)
		{
			ret = node;
		}
		else
		{
			uint32_t childCount = node->GetChildCount();
			for (uint32_t i = 0; (i < childCount) && (ret == nullptr); i++)
			{
				ret = Node::Find(node->GetChild(i), pName);
			}
		}

		return ret;
	}

	void Node::RemoveByName(NodePtr node, const wchar_t* pName)
	{
		uint32_t childCount = node->GetChildCount();
		for (uint32_t i = 0; i < childCount; i++)
		{
			Node::RemoveByName(node->GetChild(i), pName);
		}

		NodePtr parent = node->GetParent();
		if ((parent != nullptr) && (wcscmp(node->GetName(), pName) == 0))
		{
			auto it = std::find(parent->impl->childs.begin(), parent->impl->childs.end(), node);
			VE_ASSERT(it != parent->impl->childs.end());
			parent->impl->childs.erase(it);
		}
	}

	void Node::RemoveByGroup(NodePtr node, uint32_t groupMask)
	{
		uint32_t childCount = node->GetChildCount();
		for (uint32_t i = 0; i < childCount; i++)
		{
			Node::RemoveByGroup(node->GetChild(i), groupMask);
		}

		NodePtr parent = node->GetParent();
		if ((parent != nullptr) && (node->GetGroupFlags() & groupMask))
		{
			auto it = std::find(parent->impl->childs.begin(), parent->impl->childs.end(), node);
			VE_ASSERT(it != parent->impl->childs.end());
			parent->impl->childs.erase(it);
		}
	}

	const glm::mat4& Node::GetLocalMatrix() const
	{
		return impl->localMatrix;
	}

	const Transform& Node::GetLocalTransform() const
	{
		return impl->localTransform;
	}

	void Node::SetLocalTransform(const Transform& transform)
	{
		SetLocalTransform(transform.scale, transform.rotation, transform.translation);
	}

	void Node::SetLocalTransform(const glm::vec3& scale, const glm::quat& rotation, const glm::vec3 translation)
	{
		impl->localTransform.scale = scale;
		impl->localTransform.rotation = rotation;
		impl->localTransform.translation = translation;

		glm::mat4 tMat = glm::translate(glm::mat4(1.0), impl->localTransform.translation);
		glm::mat4 rMat = glm::toMat4(impl->localTransform.rotation);
		glm::mat4 sMat = glm::scale(glm::mat4(1.0), impl->localTransform.scale);

		impl->localMatrix = tMat * rMat * sMat;
	}

	const glm::mat4& Node::GetWorldMatrix() const
	{
		return impl->worldMatrix;
	}

	const Transform& Node::GetWorldTransform() const
	{
		return impl->worldTransform;
	}

	NodeAttributePtr Node::GetAttribute()
	{
		return impl->attribute;
	}

	void Node::SetAttribute(NodePtr node, NodeAttributePtr attribute)
	{
		node->impl->attribute = attribute;
		attribute->SetOwnerNode(node);
	}

	void Node::Update()
	{
		NodePtr parent = impl->parent.lock();

		if (parent != nullptr)
		{
			Update(parent->GetWorldMatrix());
		}
		else
		{
			Update(glm::mat4(1.0f));
		}
	}

	void Node::Update(const glm::mat4& worldMatrix)
	{
		// ----------------------------------------------------------------------------------------------------
		// ワールド行列を求める
		// ----------------------------------------------------------------------------------------------------

		impl->worldMatrix = worldMatrix * impl->localMatrix;

		// ----------------------------------------------------------------------------------------------------
		// ワールド行列を分解
		// ----------------------------------------------------------------------------------------------------

		glm::vec3 skew;
		glm::vec4 perspective;

		glm::decompose(impl->worldMatrix, impl->worldTransform.scale, impl->worldTransform.rotation, impl->worldTransform.translation, skew, perspective);

		// ----------------------------------------------------------------------------------------------------
		// 子を更新
		// ----------------------------------------------------------------------------------------------------

		if (impl->childs.empty() == false)
		{
			auto it_begin = impl->childs.begin();
			auto it_end = impl->childs.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				(*it)->Update(impl->worldMatrix);
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// アトリビュートを更新
		// ----------------------------------------------------------------------------------------------------

		if (impl->attribute != nullptr)
		{
			impl->attribute->Update(impl->worldMatrix);
		}
	}

	void Node::Draw(
		const Frustum& frustum,
		uint32_t frameIndex,
		collection::DynamicContainer<OpacityDrawSet>& opacityDrawSets,
		collection::DynamicContainer<TransparencyDrawSet>& transparencyDrawSets,
		uint32_t debugFlags, DebugRenderer* pDebugRenderer)
	{
		if (impl->attribute->Draw(frustum, frameIndex, opacityDrawSets, transparencyDrawSets) == true)
		{
			if (debugFlags != 0)
			{
				impl->attribute->DebugDraw(debugFlags, pDebugRenderer);
			}
		}

		if (impl->childs.empty() == false)
		{
			auto it_begin = impl->childs.begin();
			auto it_end = impl->childs.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				(*it)->Draw(frustum, frameIndex, opacityDrawSets, transparencyDrawSets, debugFlags, pDebugRenderer);
			}
		}
	}

	void Node::DrawShadow(
		const Sphere& sphere,
		uint32_t frameIndex,
		collection::DynamicContainer<ShadowDrawSet>& shadowDrawSets,
		AABB& aabb)
	{
		impl->attribute->DrawShadow(sphere, frameIndex, shadowDrawSets, aabb);

		if (impl->childs.empty() == false)
		{
			auto it_begin = impl->childs.begin();
			auto it_end = impl->childs.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				(*it)->DrawShadow(sphere, frameIndex, shadowDrawSets, aabb);
			}
		}
	}

}