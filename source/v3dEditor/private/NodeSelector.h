#pragma once

namespace ve {

	class NodeSelector final
	{
	public:
		static NodeSelector* Create();

		NodeSelector();
		~NodeSelector();

		void Destroy();

		void Clear();
		void Add(NodePtr node);
		void Remove(NodePtr node);

		NodePtr Find(uint32_t key);
		NodePtr GetFound();
		void SetFound(NodePtr node);

	private:
		uint32_t m_Index;
		collection::Map<uint32_t, NodePtr> m_Map;
		collection::Map<NodePtr, collection::Map<uint32_t, NodePtr>::iterator> m_RemoveMap;
		collection::Vector<uint32_t> m_UnusedKeys;
		NodePtr m_Found;

		VE_DECLARE_ALLOCATOR
	};

}