#pragma once

namespace ve {

	class DynamicBuffer final
	{
	public:
		static DynamicBuffer* Create(DeviceContextPtr deviceContext, V3DFlags usageFlags, uint64_t size, V3DFlags stageMask, V3DFlags accessMask, const wchar_t* pDebugName);
		void Destroy();

		DynamicBuffer();
		~DynamicBuffer();

		void* Map();
		void Unmap();

		IV3DBuffer* GetNativeBufferPtr();
		uint32_t GetNativeRangeSize() const;

	private:
		DeviceContextPtr m_DeviceContext;
		V3DFlags m_StageMask;
		V3DFlags m_AccessMask;
		uint64_t m_CopySize;
		uint32_t m_RangeSize;
		uint8_t m_UpdateMask;
		IV3DBuffer* m_pHostBuffer;
		ResourceAllocation m_HostBufferAllocation;
		IV3DBuffer* m_pDeviceBuffer;
		ResourceAllocation m_DeviceBufferAllocation;
		UpdatingHandlePtr m_UpdatingHandle;

		bool Initialize(DeviceContextPtr deviceContext, V3DFlags usageFlags, uint64_t size, V3DFlags stageMask, V3DFlags accessMask, const wchar_t* pDebugName);

		VE_DECLARE_ALLOCATOR
	};

}