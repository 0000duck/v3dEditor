#pragma once

namespace ve {

	struct ResourceMemoryDesc
	{
		V3D_RESOURCE_TYPE type;
		V3DFlags propertyFlags;
		uint64_t size;
		uint64_t alignment;
	};

	class ResourceMemory
	{
	public:
		static ResourceMemory* Create(IV3DDevice* pDevice, const ResourceMemoryDesc& desc, size_t index);
		void Destroy();

		ResourceMemory();
		~ResourceMemory();

		const ResourceMemoryDesc& GetDesc() const;

		bool IsEmpty() const;
		uint64_t GetFreeSize() const;

		void UpdateAlignment(uint64_t alignment);

		ResourceAllocation Allocate(IV3DResource* pResource);
		void Free(ResourceAllocation handle);

		size_t GetIndex() const;
		size_t UpdateIndex(size_t index);

	private:
		struct NodeSort
		{
			bool operator() (const ResourceAllocation lh, const ResourceAllocation rh) const;
		};

		IV3DDevice* m_pDevice;
		IV3DResourceMemory* m_pMemory;
		ResourceMemoryDesc m_Desc;
		uint64_t m_FreeSize;
		size_t m_Index;

		ResourceAllocationT* m_pNodeTop;
		ResourceAllocationT* m_pNodeBottom;
		collection::Vector<ResourceAllocationT*> m_UnusedNodes;
		collection::Vector<ResourceAllocationT*> m_NodePool;

		bool Initialize(IV3DDevice* pDevice, const ResourceMemoryDesc& desc, size_t index);

		ResourceAllocation AllocateNode();
		ResourceAllocation FreeNode(ResourceAllocation pNode, bool down);

		VE_DECLARE_ALLOCATOR
	};

}
