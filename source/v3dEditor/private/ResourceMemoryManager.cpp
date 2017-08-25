#include "ResourceMemoryManager.h"
#include "ResourceAllocation.h"
#include "Device.h"
#include "Logger.h"

namespace ve {

	/**********************************/
	/* public - ResourceMemoryManager */
	/**********************************/

	ResourceMemoryManager* ResourceMemoryManager::Create(LoggerPtr logger, Device* pDevice)
	{
		return VE_NEW_T(ResourceMemoryManager, logger, pDevice);
	}

	ResourceMemoryManager::ResourceMemoryManager(LoggerPtr logger, Device* pDevice) :
		m_pDevice(nullptr)
	{
		VE_ASSERT(pDevice != nullptr);

		m_Logger = logger;
		m_pDevice = pDevice;
	}

	ResourceMemoryManager::~ResourceMemoryManager()
	{
		VE_ASSERT(m_UsedBuffers.size() == 0);
		VE_ASSERT(m_UsedImages.size() == 0);

		FreeCollection(m_UsedBuffers);
		FreeCollection(m_UnusedBuffers);

		FreeCollection(m_UsedImages);
		FreeCollection(m_UnusedImages);

		m_pDevice = nullptr;
	}

	void ResourceMemoryManager::Destroy()
	{
		VE_DELETE_THIS_T(this, ResourceMemoryManager);
	}

	ResourceAllocation ResourceMemoryManager::Allocate(IV3DResource* pResource, V3DFlags propertyFlags)
	{
		LockGuard<Mutex> lock(m_Mutex);

		const V3DResourceDesc& resourceDesc = pResource->GetResourceDesc();
		uint64_t minMemorySize;

		collection::Vector<ResourceMemory*>* pUsedResources;
		collection::Vector<ResourceMemory*>* pUnusedResources[2];

		switch (resourceDesc.type)
		{
		case V3D_RESOURCE_TYPE_BUFFER:
			pUsedResources = &m_UsedBuffers;
			pUnusedResources[0] = &m_UnusedBuffers;
			pUnusedResources[1] = &m_UnusedImages;
			minMemorySize = MIN_BUFFER_MEMORY_SIZE;
			break;
		case V3D_RESOURCE_TYPE_IMAGE:
			pUsedResources = &m_UsedImages;
			pUnusedResources[0] = &m_UnusedImages;
			pUnusedResources[1] = &m_UnusedBuffers;
			minMemorySize = MIN_IMAGE_MEMORY_SIZE;
			break;

		default:
			VE_ASSERT(0);
		}

		// ----------------------------------------------------------------------------------------------------
		// 使用中のメモリから空きを探す
		// ----------------------------------------------------------------------------------------------------

		auto it_used_begin = pUsedResources->begin();
		auto it_used_end = pUsedResources->end();

		ResourceAllocation handle = nullptr;

		for (auto it_used = it_used_begin; (it_used != it_used_end) && (handle == nullptr); ++it_used)
		{
			ResourceMemory* pMemory = (*it_used);
			const ResourceMemoryDesc& memoryDesc = pMemory->GetDesc();

			VE_ASSERT(pMemory->IsEmpty() == false);

			if ((memoryDesc.propertyFlags != propertyFlags) ||
				(memoryDesc.alignment != resourceDesc.memoryAlignment))
			{
				continue;
			}

			handle = pMemory->Allocate(pResource);
		}

		if (handle != nullptr)
		{
			return handle;
		}

		// ----------------------------------------------------------------------------------------------------
		// 使用されていないメモリから空きを探す
		// ----------------------------------------------------------------------------------------------------

		auto it_unused_begin = pUnusedResources[0]->begin();
		auto it_unused_end = pUnusedResources[0]->end();

		for (auto it_unused = it_unused_begin; (it_unused != it_unused_end) && (handle == nullptr); ++it_unused)
		{
			ResourceMemory* pMemory = (*it_unused);
			const ResourceMemoryDesc& memoryDesc = pMemory->GetDesc();

			if ((memoryDesc.propertyFlags != propertyFlags) ||
				(memoryDesc.size < resourceDesc.memorySize))
			{
				continue;
			}

			pMemory->UpdateAlignment(resourceDesc.memoryAlignment);
			handle = pMemory->Allocate(pResource);
		}

		if (handle != nullptr)
		{
			ResourceMemory* pLastUnusedMemory = pUnusedResources[0]->back();

			// 未使用リストの最後の要素と入れ替え
			(*pUnusedResources[0])[handle->pOwner->GetIndex()] = pLastUnusedMemory;
			pUnusedResources[0]->pop_back();

			// インデックスを更新
			size_t newUnusedIndex = handle->pOwner->UpdateIndex(pUsedResources->size());
			if (pLastUnusedMemory != handle->pOwner)
			{
				pLastUnusedMemory->UpdateIndex(newUnusedIndex);
			}

			// 使用中リストの最後に追加
			pUsedResources->push_back(handle->pOwner);

			return handle;
		}

		// ----------------------------------------------------------------------------------------------------
		// 必要なメモリサイズを求める
		// ----------------------------------------------------------------------------------------------------

		uint64_t memorySize;

		if (minMemorySize < resourceDesc.memorySize)
		{
			memorySize = (resourceDesc.memorySize + resourceDesc.memoryAlignment - 1) / resourceDesc.memoryAlignment * resourceDesc.memoryAlignment;
		}
		else
		{
			memorySize = minMemorySize;
		}

		// ----------------------------------------------------------------------------------------------------
		// リソースのサイズ分、未使用のメモリを破棄する
		// ----------------------------------------------------------------------------------------------------

		uint64_t purgedSize = 0;
		uint64_t totalPurgedSize = 0;

		for (uint32_t i = 0; i < 2; i++)
		{
			// リソースメモリのインデックスが変わるとまずいので、底から解放していく。
			// メモリプロパティが一致していなくても ( DEVICE or HOST ) メモリを解放する。
			// ただしメモリプロパティが一致しない場合は purgeSize はカウントしない。
			while ((pUnusedResources[i]->empty() == false) && (purgedSize < memorySize))
			{
				ResourceMemory* pMemory = pUnusedResources[i]->back();
				const ResourceMemoryDesc& memoryDesc = pMemory->GetDesc();

				VE_ASSERT(pMemory->IsEmpty() == true);

				if (((memoryDesc.propertyFlags & ResourceMemoryManager::PurgeMemoryMask) & propertyFlags))
				{
					purgedSize += memoryDesc.size;
				}

				totalPurgedSize += memoryDesc.size;

				pUnusedResources[i]->pop_back();
				pMemory->Destroy();
			}
		}

		if ((purgedSize > 0) || (totalPurgedSize > 0))
		{
			float purgedSizeF = static_cast<float>(purgedSize) / (1024.0f * 1024.0f);
			float totalPurgedSizeF = static_cast<float>(totalPurgedSize) / (1024.0f * 1024.0f);
			m_Logger->PrintA(Logger::TYPE_DEBUG, "Memory purged : Size[%.3f] TotalSize(%.3f)", purgedSizeF, totalPurgedSizeF);
		}

		if (m_pDevice->GetNativeDevicePtr()->GetCaps().maxResourceMemoryCount == m_pDevice->GetNativeDevicePtr()->GetStatistics().resourceMemoryCount)
		{
			// これ以上リソースメモリを確保できない
			m_Logger->PrintA(Logger::TYPE_ERROR, "Maximum number(%u) of allocated memory has been exceeded", m_pDevice->GetNativeDevicePtr()->GetCaps().maxResourceMemoryCount);
			return nullptr;
		}

		// ----------------------------------------------------------------------------------------------------
		// 新規にメモリを追加する
		// ----------------------------------------------------------------------------------------------------

		ResourceMemoryDesc resourceMemoryDesc;
		resourceMemoryDesc.type = resourceDesc.type;
		resourceMemoryDesc.propertyFlags = propertyFlags;
		resourceMemoryDesc.size = memorySize;
		resourceMemoryDesc.alignment = resourceDesc.memoryAlignment;

		ResourceMemory* pResourceMemory = ResourceMemory::Create(m_pDevice->GetNativeDevicePtr(), resourceMemoryDesc, static_cast<uint32_t>(pUsedResources->size()));
		if (pResourceMemory == nullptr)
		{
			StringA propertyFlagsString;
			ToString_MemoryProperty(propertyFlags, propertyFlagsString);

			m_Logger->PrintA(Logger::TYPE_ERROR, "Memory allocation failed : Type[%s] Properties[%s] Size[%I64u] Alignment[%I64u]",
				ToString_ResourceType(resourceDesc.type),
				propertyFlagsString.c_str(),
				memorySize,
				resourceDesc.memoryAlignment);

			return nullptr;
		}

		pUsedResources->push_back(pResourceMemory);

		handle = pResourceMemory->Allocate(pResource);
		VE_ASSERT(handle != nullptr);

		// ----------------------------------------------------------------------------------------------------

		return handle;
	}

	void ResourceMemoryManager::Free(ResourceAllocation handle)
	{
		LockGuard<Mutex> lock(m_Mutex);

		VE_ASSERT(handle != nullptr);

		handle->pOwner->Free(handle);

		if (handle->pOwner->IsEmpty() == true)
		{
			collection::Vector<ResourceMemory*>* pUsedResources;
			collection::Vector<ResourceMemory*>* pUnusedResources;

			switch (handle->pOwner->GetDesc().type)
			{
			case V3D_RESOURCE_TYPE_BUFFER:
				pUsedResources = &m_UsedBuffers;
				pUnusedResources = &m_UnusedBuffers;
				break;
			case V3D_RESOURCE_TYPE_IMAGE:
				pUsedResources = &m_UsedImages;
				pUnusedResources = &m_UnusedImages;
				break;

			default:
				VE_ASSERT(0);
			}

			VE_ASSERT(pUsedResources->size() > 0);

			ResourceMemory* pUsedLastMemory = pUsedResources->back();

			// 使用中リストの最後の要素と入れ替え
			(*pUsedResources)[handle->pOwner->GetIndex()] = pUsedLastMemory;
			pUsedResources->pop_back();

			// インデックスを更新
			size_t newUsedIndex = handle->pOwner->UpdateIndex(pUnusedResources->size());
			if (pUsedLastMemory != handle->pOwner)
			{
				pUsedLastMemory->UpdateIndex(newUsedIndex);
			}

			// 未使用リストに追加
			pUnusedResources->push_back(handle->pOwner);

			pUsedLastMemory = nullptr;
		}
	}

	void ResourceMemoryManager::Dump()
	{
		uint64_t deviceMemorySize = 0;
		uint64_t hostMemorySize = 0;
		uint64_t allocateMemoryCount = m_UsedBuffers.size() + m_UnusedBuffers.size() + m_UsedImages.size() + m_UnusedImages.size();

		ResourceMemoryManager::Dump(m_Logger, "BI", m_UsedBuffers, deviceMemorySize, hostMemorySize);
		ResourceMemoryManager::Dump(m_Logger, "BU", m_UnusedBuffers, deviceMemorySize, hostMemorySize);
		ResourceMemoryManager::Dump(m_Logger, "II", m_UsedImages, deviceMemorySize, hostMemorySize);
		ResourceMemoryManager::Dump(m_Logger, "IU", m_UnusedImages, deviceMemorySize, hostMemorySize);

		float deviceMemorySizeF = static_cast<float>(deviceMemorySize) / (1024.0f * 1024.0f);
		float hostMemorySizeF = static_cast<float>(hostMemorySize) / (1024.0f * 1024.0f);

		m_Logger->PrintA(Logger::TYPE_DEBUG, "DeviceMemorySize[%.3f mb] HostMemorSize[%.3f mb] AllocateMemoryCount[%I64u]", deviceMemorySizeF, hostMemorySizeF, allocateMemoryCount);
	}

	void ResourceMemoryManager::Dump(LoggerPtr logger, const char* pType, collection::Vector<ResourceMemory*>& resourceMemories, uint64_t& deviceMemorySize, uint64_t& hostMemorySize)
	{
		auto it_begin = resourceMemories.begin();
		auto it_end = resourceMemories.end();

		for (auto it = it_begin; it != it_end; ++it)
		{
			const ResourceMemoryDesc& desc = (*it)->GetDesc();

			StringA propertyFlagsString;
			ToString_MemoryProperty(desc.propertyFlags, propertyFlagsString);

			VE_ASSERT(desc.size >= (*it)->GetFreeSize());
			float uasedMemorySize = static_cast<float>(desc.size - (*it)->GetFreeSize()) / (1024.0f * 1024.0f);
			float totalMemorySize = static_cast<float>(desc.size) / (1024.0f * 1024.0f);

			logger->PrintA(Logger::TYPE_DEBUG, "%s %u : Properties[%s] Size[%.3f/%.3f mb] Alignment[%I64u byte]",
				pType,
				(*it)->GetIndex(),
				propertyFlagsString.c_str(),
				uasedMemorySize,
				totalMemorySize,
				desc.alignment);

			if (desc.propertyFlags & V3D_MEMORY_PROPERTY_DEVICE_LOCAL)
			{
				deviceMemorySize += desc.size;
			}
			else
			{
				hostMemorySize += desc.size;
			}
		}
	}

	void ResourceMemoryManager::FreeCollection(collection::Vector<ResourceMemory*>& resources)
	{
		if (resources.empty() == false)
		{
			auto it_begin = resources.begin();
			auto it_end = resources.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				(*it)->Destroy();
			}

			resources.clear();
		}
	}

}