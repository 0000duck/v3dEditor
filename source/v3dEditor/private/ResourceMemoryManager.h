#pragma once

#include "ResourceMemory.h"

namespace ve {

	class Device;

	class ResourceMemoryManager
	{
	public:
		static ResourceMemoryManager* Create(LoggerPtr logger, Device* pDevice);

		ResourceMemoryManager(LoggerPtr logger, Device* pDevice);
		~ResourceMemoryManager();

		void Destroy();

		ResourceAllocation Allocate(IV3DResource* pResource, V3DFlags propertyFlags);
		void Free(ResourceAllocation handle);

		void Dump();

	private:
		static constexpr V3DFlags PurgeMemoryMask = V3D_MEMORY_PROPERTY_DEVICE_LOCAL | V3D_MEMORY_PROPERTY_HOST_VISIBLE;

		LoggerPtr m_Logger;

		Device* m_pDevice;
		Mutex m_Mutex;

		collection::Vector<ResourceMemory*> m_UsedBuffers;
		collection::Vector<ResourceMemory*> m_UnusedBuffers;

		collection::Vector<ResourceMemory*> m_UsedImages;
		collection::Vector<ResourceMemory*> m_UnusedImages;

		static void FreeCollection(collection::Vector<ResourceMemory*>& resources);
		static void Dump(LoggerPtr logger, const char* pType, collection::Vector<ResourceMemory*>& resourceMemories, uint64_t& deviceMemorySize, uint64_t& hostMemorySize);

		VE_DECLARE_ALLOCATOR
	};

}