#include "ResourceMemory.h"
#include "ResourceAllocation.h"

namespace ve {

	bool ResourceMemory::NodeSort::operator() (const ResourceAllocation lh, const ResourceAllocation rh) const
	{
		if (lh->exist < rh->exist)
		{
			return false;
		}

		return (lh->size > rh->size);
	}

	ResourceMemory* ResourceMemory::Create(IV3DDevice* pDevice, const ResourceMemoryDesc& desc, size_t index)
	{
		ResourceMemory* pResourceMemory = VE_NEW_T(ResourceMemory);
		if (pResourceMemory == nullptr)
		{
			return nullptr;
		}

		if (pResourceMemory->Initialize(pDevice, desc, index) == false)
		{
			pResourceMemory->Destroy();
			return nullptr;
		}

		return pResourceMemory;
	}

	void ResourceMemory::Destroy()
	{
		VE_DELETE_THIS_T(this, ResourceMemory);
	}

	ResourceMemory::ResourceMemory() :
		m_pDevice(nullptr),
		m_pMemory(nullptr),
		m_Desc({}),
		m_FreeSize(0),
		m_Index(~0U),
		m_pNodeTop(nullptr),
		m_pNodeBottom(nullptr)
	{
	}

	ResourceMemory::~ResourceMemory()
	{
		if (m_NodePool.empty() == false)
		{
			auto it_begin = m_NodePool.begin();
			auto it_end = m_NodePool.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				ResourceAllocationT* pNode = *it;
				VE_DELETE_T(pNode, ResourceAllocationT);
			}
		}

		if (m_pNodeTop != nullptr)
		{
			ResourceAllocationT* pNode = m_pNodeTop;
			while (pNode != nullptr)
			{
				ResourceAllocationT* pNextNode = pNode->pNext;
				VE_DELETE_T(pNode, ResourceAllocationT);
				pNode = pNextNode;
			}

			m_pNodeTop = nullptr;
		}

		VE_RELEASE(m_pMemory);
		VE_RELEASE(m_pDevice);
	}

	const ResourceMemoryDesc& ResourceMemory::GetDesc() const
	{
		return m_Desc;
	}

	bool ResourceMemory::IsEmpty() const
	{
		return (m_Desc.size == m_FreeSize);
	}

	uint64_t ResourceMemory::GetFreeSize() const
	{
		return m_FreeSize;
	}

	void ResourceMemory::UpdateAlignment(uint64_t alignment)
	{
		VE_ASSERT(IsEmpty() == true);

		m_Desc.alignment = alignment;
	}

	ResourceAllocation ResourceMemory::Allocate(IV3DResource* pResource)
	{
		if (m_UnusedNodes.empty() == true)
		{
			return nullptr;
		}

		// ----------------------------------------------------------------------------------------------------
		// メモリを確保
		// ----------------------------------------------------------------------------------------------------

		/**************************/
		/* 空いているノードを探す */
		/**************************/

		const V3DResourceDesc& resourceDesc = pResource->GetResourceDesc();

		VE_ASSERT(resourceDesc.type == m_Desc.type);
		VE_ASSERT(resourceDesc.memoryAlignment == m_Desc.alignment);

		uint64_t memorySize = ((resourceDesc.memorySize + m_Desc.alignment - 1) / m_Desc.alignment) * m_Desc.alignment;

		ResourceAllocationT* pNode = nullptr;
		size_t unusedNodeIndex = 0;

		ResourceAllocationT** ppNode = m_UnusedNodes.data();
		ResourceAllocationT** ppNodeEnd = ppNode + m_UnusedNodes.size();

		while ((ppNode != ppNodeEnd) && (pNode == nullptr))
		{
			ResourceAllocationT* pTempNode = *ppNode;

			if (pTempNode->size >= memorySize)
			{
				pNode = pTempNode;
			}

			unusedNodeIndex++;
			ppNode++;
		}

		if (pNode == nullptr)
		{
			return nullptr;
		}

		// 1 余計にカウントされるはずなので -1 しておく
		VE_ASSERT(unusedNodeIndex >= 1);
		unusedNodeIndex -= 1;

		/**************************/
		/* ノードの使用状況を更新 */
		/**************************/

		uint64_t downSize = 0;
		if (pNode->size > memorySize)
		{
			// 下方向に分割
			ResourceAllocationT* pNewNode = AllocateNode();
			VE_ASSERT(pNode != nullptr);

			pNewNode->pOwner = this;
			pNewNode->exist = true;
			pNewNode->used = false;
			pNewNode->offset = pNode->offset + memorySize;
			pNewNode->size = pNode->size - (pNewNode->offset - pNode->offset);

			downSize = pNewNode->size;

			if (pNode == m_pNodeBottom)
			{
				VE_ASSERT(pNode->pNext == nullptr);

				pNode->pNext = pNewNode;

				pNewNode->pPrev = pNode;
				pNewNode->pNext = nullptr;

				m_pNodeBottom = pNewNode;
			}
			else
			{
				VE_ASSERT(pNode->pNext != nullptr);

				pNewNode->pPrev = pNode;
				pNewNode->pNext = pNode->pNext;

				pNode->pNext->pPrev = pNewNode;
				pNode->pNext = pNewNode;
			}

			m_UnusedNodes.push_back(pNewNode);
		}

		/************************/
		/* 未使用リストから削除 */
		/************************/

		VE_ASSERT(m_UnusedNodes[unusedNodeIndex] == pNode);

		m_UnusedNodes[unusedNodeIndex] = m_UnusedNodes.back();
		m_UnusedNodes[m_UnusedNodes.size() - 1] = pNode;
		m_UnusedNodes.pop_back();

		pNode->used = true;
		pNode->size = memorySize;

		VE_ASSERT(m_FreeSize >= memorySize);

		m_FreeSize -= memorySize;

		// ----------------------------------------------------------------------------------------------------
		// リソースをメモリにバインド
		// ----------------------------------------------------------------------------------------------------

		V3D_RESULT result = m_pDevice->BindResourceMemory(m_pMemory, pResource, pNode->offset);
		if (result != V3D_OK)
		{
			Free(pNode);
			return nullptr;
		}

		return pNode;
	}

	void ResourceMemory::Free(ResourceAllocation handle)
	{
		// ----------------------------------------------------------------------------------------------------
		// 空いているノードを結合
		// ----------------------------------------------------------------------------------------------------

		ResourceAllocationT* pCompNode;
		uint64_t newOffset = handle->offset;
		uint64_t newSize = handle->size;
		size_t invalidNodeCount = 0;

		// 上方向
		pCompNode = handle->pPrev;
		while (pCompNode != nullptr)
		{
			if (pCompNode->used == false)
			{
				newOffset = pCompNode->offset;
				newSize += pCompNode->size;

				invalidNodeCount++;

				pCompNode = FreeNode(pCompNode, false);
			}
			else
			{
				pCompNode = nullptr;
			}
		}

		// 下方向
		pCompNode = handle->pNext;
		while (pCompNode != nullptr)
		{
			if (pCompNode->used == false)
			{
				newSize += pCompNode->size;

				invalidNodeCount++;

				pCompNode = FreeNode(pCompNode, true);
			}
			else
			{
				pCompNode = nullptr;
			}
		}

		m_FreeSize += handle->size;

		handle->used = false;
		handle->offset = newOffset;
		handle->size = newSize;

		// ----------------------------------------------------------------------------------------------------
		// 未使用リストに追加してソート
		// ----------------------------------------------------------------------------------------------------

		m_UnusedNodes.push_back(handle);

		std::sort(m_UnusedNodes.begin(), m_UnusedNodes.end(), ResourceMemory::NodeSort());

		// ----------------------------------------------------------------------------------------------------
		// 結合され無効になったノードを削除
		// ----------------------------------------------------------------------------------------------------

		if (invalidNodeCount != 0)
		{
			m_UnusedNodes.resize(m_UnusedNodes.size() - invalidNodeCount);
		}

		// ----------------------------------------------------------------------------------------------------
	}

	size_t ResourceMemory::GetIndex() const
	{
		return m_Index;
	}

	size_t ResourceMemory::UpdateIndex(size_t index)
	{
		size_t oldIndex = m_Index;

		m_Index = index;

		return oldIndex;
	}

	bool ResourceMemory::Initialize(IV3DDevice* pDevice, const ResourceMemoryDesc& desc, size_t index)
	{
		pDevice->AddRef();
		m_pDevice = pDevice;

		m_Desc.type = desc.type;
		m_Desc.propertyFlags = desc.propertyFlags;
		m_Desc.size = ((desc.size + desc.alignment - 1) / desc.alignment) * desc.alignment;
		m_Desc.alignment = desc.alignment;

		m_Index = index;

		V3D_RESULT result = m_pDevice->AllocateResourceMemory(m_Desc.propertyFlags, m_Desc.size, &m_pMemory);
		if (result != V3D_OK)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// 最初のノードを作成
		// ----------------------------------------------------------------------------------------------------

		ResourceAllocationT* pNode = VE_NEW_T(ResourceAllocationT);
		if (pNode == nullptr)
		{
			return false;
		}

		pNode->pOwner = this;
		pNode->exist = true;
		pNode->used = false;
		pNode->offset = 0;
		pNode->size = m_Desc.size;
		pNode->pPrev = nullptr;
		pNode->pNext = nullptr;

		m_pNodeTop = pNode;
		m_pNodeBottom = pNode;

		m_UnusedNodes.push_back(pNode);

		m_FreeSize = m_Desc.size;

		return true;
	}

	ResourceAllocation ResourceMemory::AllocateNode()
	{
		if (m_NodePool.empty() == false)
		{
			ResourceAllocationT* pNode = m_NodePool.back();
			m_NodePool.pop_back();
			return pNode;
		}

		return VE_NEW_T(ResourceAllocationT);
	}

	ResourceAllocation ResourceMemory::FreeNode(ResourceAllocation handle, bool down)
	{
#ifdef _DEBUG
		if ((m_pNodeTop == handle) && (m_pNodeBottom == handle))
		{
			VE_ASSERT(0);
		}
#endif //_DEBUG

		if ((m_pNodeTop == handle) && (m_pNodeBottom != handle))
		{
			m_pNodeTop = handle->pNext;
			m_pNodeTop->pPrev = nullptr;
		}
		else if ((m_pNodeTop != handle) && (m_pNodeBottom == handle))
		{
			m_pNodeBottom = handle->pPrev;
			m_pNodeBottom->pNext = nullptr;
		}
		else
		{
			handle->pPrev->pNext = handle->pNext;
			handle->pNext->pPrev = handle->pPrev;
		}

		ResourceAllocationT* pNextNode = (down == true) ? handle->pNext : handle->pPrev;

		handle->exist = false;
		handle->used = false;
		handle->offset = 0;
		handle->size = 0;
		handle->pPrev = nullptr;
		handle->pNext = nullptr;

		m_NodePool.push_back(handle);

		return pNextNode;
	}

}