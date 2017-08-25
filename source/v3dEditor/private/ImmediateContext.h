#pragma once

namespace ve {

	class ResourceMemoryManager;

	class ImmediateContext final
	{
	public:
		IV3DCommandBuffer* Begin();
		void End();

		bool Upload(
			const void* pSrc, uint64_t srcSize,
			V3DFlags dstBufferUsageFlags, V3DFlags dstStageMask, V3DFlags dstAccessMask, IV3DBuffer** ppDstBuffer, ResourceAllocation* pDstBufferAllocation,
			const wchar_t* pDebugName);

		bool Upload(IV3DImage* pDstImage, const void* pSrc, uint64_t srcSize, uint32_t rangeCount, const V3DCopyBufferToImageRange* pRanges);

		bool Download(V3DFlags srcStageMask, V3DFlags srcAccessMask, IV3DBuffer* pSrcBuffer, IV3DBuffer** ppDstBuffer, ResourceAllocation* pDstBufferAllocation);

	private:
		struct Staging
		{
			IV3DBuffer* pBuffer;
			ResourceAllocation bufferAllocation;
		};

		IV3DDevice* m_pV3DDevice;
		ResourceMemoryManager* m_pResourceMemoryManager;
		IV3DQueue* m_pQueue;

		Mutex m_Mutex;
		IV3DCommandBuffer* m_pCommandBuffer;
		IV3DFence* m_pFence;

		collection::Vector<ImmediateContext::Staging> m_Stagings;

		static ImmediateContext* Create(IV3DDevice* pV3DDevice, IV3DQueue* pQueue, ResourceMemoryManager* pResourceMemoryManager);
		void Destroy();

		ImmediateContext();
		~ImmediateContext();

		bool Initialize(IV3DDevice* pV3DDevice, IV3DQueue* pQueue, ResourceMemoryManager* pResourceMemoryManager);

		friend class Device;

		VE_DECLARE_ALLOCATOR
	};

}