#include "NodeAttribute.h"
#include "Node.h"

namespace ve {

	struct NodeAttribute::Impl
	{
		WeakPtr<Node> owner;

		VE_DECLARE_ALLOCATOR
	};

	NodeAttribute::NodeAttribute() :
		impl(VE_NEW_T(NodeAttribute::Impl))
	{
	}

	NodeAttribute::~NodeAttribute()
	{
		VE_DELETE_T(impl, Impl);
	}

	NodePtr NodeAttribute::GetOwner()
	{
		return impl->owner.lock();
	}

	void NodeAttribute::SetOwnerNode(NodePtr node)
	{
		impl->owner = node;
	}

}