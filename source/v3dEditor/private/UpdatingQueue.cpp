#include "UpdatingQueue.h"
#include "DeviceContext.h"

namespace ve {

	/***************************/
	/* public - UpdatingHandle */
	/***************************/

	UpdatingHandlePtr UpdatingHandle::Create()
	{
		return std::move(std::make_shared<UpdatingHandle>());
	}

	UpdatingHandle::UpdatingHandle() :
		m_FrameMask(0)
	{
	}

	UpdatingHandle::~UpdatingHandle()
	{
	}

	/***************************/
	/* public  - UpdatingQueue */
	/***************************/

	void UpdatingQueue::Add(UpdatingHandlePtr handle, IV3DBuffer* pDstBuffer, V3DFlags dstStageMask, V3DFlags dstAccessMask, uint64_t dstRangeSize, IV3DBuffer* pSrcBuffer, uint64_t size)
	{
		VE_ASSERT(m_Disposed == false);

		LockGuard<Mutex> lock(m_Mutex);
		
		UpdatingQueue::Frame* pFrame = m_pFrameBegin;
		uint8_t frameMask = handle->m_FrameMask;
		uint32_t bit = 0x01;

		UpdatingQueue::DynamicBufferInfo info;
		info.handle = handle;
		info.pDstBuffer = pDstBuffer;
		info.dstStageMask = dstStageMask;
		info.dstAccessMask = dstAccessMask;
		info.dstOffset = 0;
		info.dstRangeSize = dstRangeSize;
		info.pSrcBuffer = pSrcBuffer;
		info.size = dstRangeSize;

		while (pFrame != m_pFrameEnd)
		{
			if ((frameMask & bit) == 0)
			{
				pDstBuffer->AddRef();
				pSrcBuffer->AddRef();

				pFrame->dynamicBuffers.push_back(info);
				frameMask |= bit;
			}

			info.dstOffset += dstRangeSize;
			bit <<= 1;
			pFrame++;
		}

		handle->m_FrameMask = frameMask;
	}

	UpdatingQueue::UpdatingQueue() :
		m_FrameIndex(0),
		m_pFrameBegin(nullptr),
		m_pFrameEnd(nullptr),
		m_pCurrentFrame(nullptr),
		m_Disposed(false)
	{
	}

	UpdatingQueue::~UpdatingQueue()
	{
		LockGuard<Mutex> lock(m_Mutex);

		UpdatingQueue::Frame* pFrame = m_pFrameBegin;
		while (pFrame != m_pFrameEnd)
		{
			UpdatingQueue::DynamicBufferInfo* pInfo = pFrame->dynamicBuffers.data();
			UpdatingQueue::DynamicBufferInfo* pInfoEnd = pInfo + pFrame->dynamicBuffers.size();

			while (pInfo != pInfoEnd)
			{
				pInfo->pSrcBuffer->Release();
				pInfo->pDstBuffer->Release();

				pInfo++;
			}

			pFrame++;
		}
	}

	UpdatingQueue* UpdatingQueue::Create(uint32_t frameCount)
	{
		UpdatingQueue* pUpdatingQueue = VE_NEW_T(UpdatingQueue);
		if (pUpdatingQueue == nullptr)
		{
			return nullptr;
		}

		if (pUpdatingQueue->Initialize(frameCount) == false)
		{
			VE_DELETE_T(pUpdatingQueue, UpdatingQueue);
			return nullptr;
		}

		return pUpdatingQueue;
	}

	void UpdatingQueue::Destroy()
	{
		VE_DELETE_THIS_T(this, UpdatingQueue);
	}

	bool UpdatingQueue::Initialize(uint32_t frameCount)
	{
		m_Frames.resize(frameCount);

		m_pFrameBegin = m_Frames.data();
		m_pFrameEnd = m_pFrameBegin + m_Frames.size();

		m_pCurrentFrame = &m_Frames[0];

		return true;
	}

	void UpdatingQueue::UpdateCurrentFrame(uint32_t frameIndex)
	{
		LockGuard<Mutex> lock(m_Mutex);

		m_pCurrentFrame = &m_Frames[frameIndex];
	}

	void UpdatingQueue::Flush(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex)
	{
		LockGuard<Mutex> lock(m_Mutex);

		uint8_t frameBit = 1 << frameIndex;

		// ダイナミックバッファーを更新
		if (m_pCurrentFrame->dynamicBuffers.empty() == false)
		{
			UpdatingQueue::DynamicBufferInfo* pInfo = m_pCurrentFrame->dynamicBuffers.data();
			UpdatingQueue::DynamicBufferInfo* pInfoEnd = pInfo + m_pCurrentFrame->dynamicBuffers.size();

			V3DPipelineBarrier pipelineBarrier;
			pipelineBarrier.dependencyFlags = 0;

			V3DBufferMemoryBarrier memoryBuffer;
			memoryBuffer.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
			memoryBuffer.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;

			while (pInfo != pInfoEnd)
			{
				if (pInfo->handle->m_FrameMask & frameBit)
				{
					pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_HOST;
					pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_TRANSFER;
					memoryBuffer.srcAccessMask = V3D_ACCESS_HOST_WRITE;
					memoryBuffer.dstAccessMask = V3D_ACCESS_TRANSFER_READ;
					memoryBuffer.pBuffer = pInfo->pSrcBuffer;
					memoryBuffer.offset = 0;
					memoryBuffer.size = pInfo->size;
					pCommandBuffer->Barrier(pipelineBarrier, memoryBuffer);

					pipelineBarrier.srcStageMask = pInfo->dstStageMask;
					pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_TRANSFER;
					memoryBuffer.srcAccessMask = pInfo->dstAccessMask;
					memoryBuffer.dstAccessMask = V3D_ACCESS_TRANSFER_READ;
					memoryBuffer.pBuffer = pInfo->pDstBuffer;
					memoryBuffer.offset = pInfo->dstOffset;
					memoryBuffer.size = pInfo->size;
					pCommandBuffer->Barrier(pipelineBarrier, memoryBuffer);

					pCommandBuffer->CopyBuffer(pInfo->pDstBuffer, pInfo->dstRangeSize * frameIndex, pInfo->pSrcBuffer, 0, pInfo->size);

					pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TRANSFER;
					pipelineBarrier.dstStageMask = pInfo->dstStageMask;
					memoryBuffer.srcAccessMask = V3D_ACCESS_TRANSFER_READ;
					memoryBuffer.dstAccessMask = pInfo->dstAccessMask;
					memoryBuffer.pBuffer = pInfo->pDstBuffer;
					memoryBuffer.offset = pInfo->dstOffset;
					memoryBuffer.size = pInfo->size;
					pCommandBuffer->Barrier(pipelineBarrier, memoryBuffer);

					pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TRANSFER;
					pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_HOST;
					memoryBuffer.srcAccessMask = V3D_ACCESS_TRANSFER_READ;
					memoryBuffer.dstAccessMask = V3D_ACCESS_HOST_WRITE;
					memoryBuffer.pBuffer = pInfo->pSrcBuffer;
					memoryBuffer.offset = 0;
					memoryBuffer.size = pInfo->size;
					pCommandBuffer->Barrier(pipelineBarrier, memoryBuffer);

					pInfo->pSrcBuffer->Release();
					pInfo->pDstBuffer->Release();

					pInfo->handle->m_FrameMask ^= frameBit;
				}

				pInfo++;
			}

			m_pCurrentFrame->dynamicBuffers.clear();
		}
	}

	void UpdatingQueue::FlushAll(IV3DCommandBuffer* pCommandBuffer)
	{
		LockGuard<Mutex> lock(m_Mutex);

		UpdatingQueue::Frame* pFrame = m_pFrameBegin;
		while (pFrame != m_pFrameEnd)
		{
			UpdatingQueue::DynamicBufferInfo* pInfo = pFrame->dynamicBuffers.data();
			UpdatingQueue::DynamicBufferInfo* pInfoEnd = pInfo + pFrame->dynamicBuffers.size();

			while (pInfo != pInfoEnd)
			{
				pInfo->pSrcBuffer->Release();
				pInfo->pDstBuffer->Release();

				pInfo->handle->m_FrameMask = 0;

				pInfo++;
			}

			pFrame->dynamicBuffers.clear();

			pFrame++;
		}
	}

	void UpdatingQueue::Dispose()
	{
		m_Disposed = true;
	}

}