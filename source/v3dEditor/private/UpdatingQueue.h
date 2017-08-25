#pragma once

#include "BufferedContainer.h"

namespace ve {

	class UpdatingHandle final
	{
	public:
		static UpdatingHandlePtr Create();

		UpdatingHandle();
		~UpdatingHandle();

	private:
		uint8_t m_FrameMask;

		friend class UpdatingQueue;
	};

	class UpdatingQueue final
	{
	public:
		void Add(UpdatingHandlePtr handle, IV3DBuffer* pDstBuffer, V3DFlags dstStageMask, V3DFlags dstAccessMask, uint64_t dstRangeSize, IV3DBuffer* pSrcBuffer, uint64_t size);

	private:
		struct DynamicBufferInfo
		{
			UpdatingHandlePtr handle;
			IV3DBuffer* pDstBuffer;
			V3DFlags dstStageMask;
			V3DFlags dstAccessMask;
			uint64_t dstOffset;
			uint64_t dstRangeSize;
			IV3DBuffer* pSrcBuffer;
			uint64_t size;
		};

		struct Frame
		{
			collection::Vector<UpdatingQueue::DynamicBufferInfo> dynamicBuffers;
		};

		Mutex m_Mutex;

		collection::Vector<UpdatingQueue::Frame> m_Frames;
		UpdatingQueue::Frame* m_pFrameBegin;
		UpdatingQueue::Frame* m_pFrameEnd;
		uint32_t m_FrameIndex;
		UpdatingQueue::Frame* m_pCurrentFrame;
		bool m_Disposed;

		UpdatingQueue();
		~UpdatingQueue();

		static UpdatingQueue* Create(uint32_t frameCount);
		void Destroy();

		bool Initialize(uint32_t frameCount);

		void UpdateCurrentFrame(uint32_t frameIndex);

		void Flush(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex);
		void FlushAll(IV3DCommandBuffer* pCommandBuffer);

		void Dispose();

		friend class DeviceContext;

		VE_DECLARE_ALLOCATOR
	};

}