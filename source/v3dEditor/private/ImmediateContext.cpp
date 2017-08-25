#include "ImmediateContext.h"
#include "Device.h"
#include "ResourceMemoryManager.h"

namespace ve {

	IV3DCommandBuffer* ImmediateContext::Begin()
	{
		m_Mutex.lock();

		m_pCommandBuffer->Begin();
		return m_pCommandBuffer;
	}

	void ImmediateContext::End()
	{
		m_pCommandBuffer->End();

		m_pFence->Reset();
		m_pQueue->Submit(1, &m_pCommandBuffer, m_pFence);
		m_pFence->Wait();

		if (m_Stagings.empty() == false)
		{
			auto it_begin = m_Stagings.begin();
			auto it_end = m_Stagings.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				VE_RELEASE(it->pBuffer);
				m_pResourceMemoryManager->Free(it->bufferAllocation);
			}

			m_Stagings.clear();
		}

		m_Mutex.unlock();
	}

	bool ImmediateContext::Upload(
		const void* pSrc, uint64_t srcSize,
		V3DFlags dstBufferUsageFlags, V3DFlags dstStageMask, V3DFlags dstAccessMask, IV3DBuffer** ppDstBuffer, ResourceAllocation* pDstBufferAllocation,
		const wchar_t* pDebugName)
	{
		// ----------------------------------------------------------------------------------------------------
		// 転送元バッファーを作成
		// ----------------------------------------------------------------------------------------------------

		ImmediateContext::Staging staging;

		V3DBufferDesc bufferDesc;
		bufferDesc.usageFlags = V3D_BUFFER_USAGE_TRANSFER_SRC;
		bufferDesc.size = srcSize;

		if (m_pV3DDevice->CreateBuffer(bufferDesc, &staging.pBuffer, VE_INTERFACE_DEBUG_NAME(L"VE_StagingBuffer")) != V3D_OK)
		{
			return false;
		}

		staging.bufferAllocation = m_pResourceMemoryManager->Allocate(staging.pBuffer, V3D_MEMORY_PROPERTY_HOST_VISIBLE);
		if (staging.bufferAllocation == nullptr)
		{
			staging.pBuffer->Release();
			return false;
		}

		void* pMemory;
		if (staging.pBuffer->Map(0, 0, &pMemory) == V3D_OK)
		{
			memcpy_s(pMemory, VE_U64_TO_U32(staging.pBuffer->GetResourceDesc().memorySize), pSrc, VE_U64_TO_U32(srcSize));
			staging.pBuffer->Unmap();
		}
		else
		{
			staging.pBuffer->Release();
			m_pResourceMemoryManager->Free(staging.bufferAllocation);
			return false;
		}

		m_Stagings.push_back(staging);

		// ----------------------------------------------------------------------------------------------------
		// 転送先バッファーを作成
		// ----------------------------------------------------------------------------------------------------

		bufferDesc.usageFlags = dstBufferUsageFlags | V3D_BUFFER_USAGE_TRANSFER_DST;
		bufferDesc.size = srcSize;

		IV3DBuffer* pDstBuffer;

		if (m_pV3DDevice->CreateBuffer(bufferDesc, &pDstBuffer, VE_INTERFACE_DEBUG_NAME(pDebugName)) != V3D_OK)
		{
			return false;
		}

		ResourceAllocation dstBufferAllocation = m_pResourceMemoryManager->Allocate(pDstBuffer, V3D_MEMORY_PROPERTY_DEVICE_LOCAL);
		if (dstBufferAllocation == nullptr)
		{
			dstBufferAllocation = m_pResourceMemoryManager->Allocate(pDstBuffer, V3D_MEMORY_PROPERTY_HOST_VISIBLE);
			if (dstBufferAllocation == nullptr)
			{
				pDstBuffer->Release();
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// 転送
		// ----------------------------------------------------------------------------------------------------

		V3DPipelineBarrier pipelineBarrier;
		pipelineBarrier.dependencyFlags = 0;

		V3DBufferMemoryBarrier bufferMemoryBarrier;
		bufferMemoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		bufferMemoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		bufferMemoryBarrier.offset = 0;
		bufferMemoryBarrier.size = 0;

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TOP_OF_PIPE;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_TRANSFER;
		bufferMemoryBarrier.srcAccessMask = 0;
		bufferMemoryBarrier.dstAccessMask = V3D_ACCESS_TRANSFER_READ;
		bufferMemoryBarrier.pBuffer = staging.pBuffer;
		m_pCommandBuffer->Barrier(pipelineBarrier, bufferMemoryBarrier);

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TOP_OF_PIPE;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_TRANSFER;
		bufferMemoryBarrier.srcAccessMask = 0;
		bufferMemoryBarrier.dstAccessMask = V3D_ACCESS_TRANSFER_WRITE;
		bufferMemoryBarrier.pBuffer = pDstBuffer;
		m_pCommandBuffer->Barrier(pipelineBarrier, bufferMemoryBarrier);

		m_pCommandBuffer->CopyBuffer(pDstBuffer, 0, staging.pBuffer, 0, srcSize);

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TRANSFER;
		pipelineBarrier.dstStageMask = dstStageMask;
		bufferMemoryBarrier.srcAccessMask = V3D_ACCESS_TRANSFER_WRITE;
		bufferMemoryBarrier.dstAccessMask = dstAccessMask;
		bufferMemoryBarrier.pBuffer = pDstBuffer;
		m_pCommandBuffer->Barrier(pipelineBarrier, bufferMemoryBarrier);

		// ----------------------------------------------------------------------------------------------------

		*ppDstBuffer = pDstBuffer;
		*pDstBufferAllocation = dstBufferAllocation;

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	bool ImmediateContext::Upload(IV3DImage* pDstImage, const void* pSrc, uint64_t srcSize, uint32_t rangeCount, const V3DCopyBufferToImageRange* pRanges)
	{
		// ----------------------------------------------------------------------------------------------------
		// バッファーを作成
		// ----------------------------------------------------------------------------------------------------

		ImmediateContext::Staging staging;

		V3DBufferDesc bufferDesc;
		bufferDesc.usageFlags = V3D_BUFFER_USAGE_TRANSFER_SRC | V3D_BUFFER_USAGE_TRANSFER_DST;
		bufferDesc.size = srcSize;

		if (m_pV3DDevice->CreateBuffer(bufferDesc, &staging.pBuffer, VE_INTERFACE_DEBUG_NAME(L"VE_StagingBuffer")) != V3D_OK)
		{
			return false;
		}

		staging.bufferAllocation = m_pResourceMemoryManager->Allocate(staging.pBuffer, V3D_MEMORY_PROPERTY_HOST_VISIBLE);
		if (staging.bufferAllocation == nullptr)
		{
			staging.pBuffer->Release();
			return false;
		}

		m_Stagings.push_back(staging);

		// ----------------------------------------------------------------------------------------------------
		// アップロード
		// ----------------------------------------------------------------------------------------------------

		V3DPipelineBarrier pipelineBarrier;
		pipelineBarrier.dependencyFlags = 0;

		V3DBufferMemoryBarrier bufferMemoryBarrier;
		bufferMemoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		bufferMemoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		bufferMemoryBarrier.pBuffer = staging.pBuffer;
		bufferMemoryBarrier.offset = 0;
		bufferMemoryBarrier.size = 0;

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TOP_OF_PIPE;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_TRANSFER;
		bufferMemoryBarrier.srcAccessMask = 0;
		bufferMemoryBarrier.dstAccessMask = V3D_ACCESS_TRANSFER_WRITE;
		m_pCommandBuffer->Barrier(pipelineBarrier, bufferMemoryBarrier);

		m_pCommandBuffer->UpdateBuffer(staging.pBuffer, 0, srcSize, pSrc);

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TRANSFER;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_TRANSFER;
		bufferMemoryBarrier.srcAccessMask = V3D_ACCESS_TRANSFER_WRITE;
		bufferMemoryBarrier.dstAccessMask = V3D_ACCESS_TRANSFER_READ;
		m_pCommandBuffer->Barrier(pipelineBarrier, bufferMemoryBarrier);

		m_pCommandBuffer->CopyBufferToImage(pDstImage, V3D_IMAGE_LAYOUT_TRANSFER_DST, staging.pBuffer, rangeCount, pRanges);

		return true;
	}

	bool ImmediateContext::Download(V3DFlags srcStageMask, V3DFlags srcAccessMask, IV3DBuffer* pSrcBuffer, IV3DBuffer** ppDstBuffer, ResourceAllocation* pDstBufferAllocation)
	{
		// ----------------------------------------------------------------------------------------------------
		// バッファーを作成
		// ----------------------------------------------------------------------------------------------------

		ImmediateContext::Staging staging;

		V3DBufferDesc bufferDesc;
		bufferDesc.usageFlags = V3D_BUFFER_USAGE_TRANSFER_DST;
		bufferDesc.size = pSrcBuffer->GetResourceDesc().memorySize;

		if (m_pV3DDevice->CreateBuffer(bufferDesc, &staging.pBuffer, VE_INTERFACE_DEBUG_NAME(L"VE_StagingBuffer")) != V3D_OK)
		{
			return false;
		}

		staging.bufferAllocation = m_pResourceMemoryManager->Allocate(staging.pBuffer, V3D_MEMORY_PROPERTY_HOST_VISIBLE);
		if (staging.bufferAllocation == nullptr)
		{
			staging.pBuffer->Release();
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// ダウンロード
		// ----------------------------------------------------------------------------------------------------

		V3DPipelineBarrier pipelineBarrier;
		pipelineBarrier.dependencyFlags = 0;

		V3DBufferMemoryBarrier bufferMemoryBarrier;
		bufferMemoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		bufferMemoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		bufferMemoryBarrier.offset = 0;
		bufferMemoryBarrier.size = 0;

		pipelineBarrier.srcStageMask = srcStageMask;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_TRANSFER;
		bufferMemoryBarrier.srcAccessMask = srcAccessMask;
		bufferMemoryBarrier.dstAccessMask = V3D_ACCESS_TRANSFER_READ;
		bufferMemoryBarrier.pBuffer = pSrcBuffer;
		m_pCommandBuffer->Barrier(pipelineBarrier, bufferMemoryBarrier);

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TOP_OF_PIPE;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_TRANSFER;
		bufferMemoryBarrier.srcAccessMask = 0;
		bufferMemoryBarrier.dstAccessMask = V3D_ACCESS_TRANSFER_WRITE;
		bufferMemoryBarrier.pBuffer = staging.pBuffer;
		m_pCommandBuffer->Barrier(pipelineBarrier, bufferMemoryBarrier);

		m_pCommandBuffer->CopyBuffer(staging.pBuffer, 0, pSrcBuffer, 0, bufferDesc.size);

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TRANSFER;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_HOST;
		bufferMemoryBarrier.srcAccessMask = V3D_ACCESS_TRANSFER_WRITE;
		bufferMemoryBarrier.dstAccessMask = V3D_ACCESS_HOST_READ;
		bufferMemoryBarrier.pBuffer = staging.pBuffer;
		m_pCommandBuffer->Barrier(pipelineBarrier, bufferMemoryBarrier);

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TRANSFER;
		pipelineBarrier.dstStageMask = srcStageMask;
		bufferMemoryBarrier.srcAccessMask = V3D_ACCESS_TRANSFER_READ;
		bufferMemoryBarrier.dstAccessMask = srcAccessMask;
		bufferMemoryBarrier.pBuffer = pSrcBuffer;
		m_pCommandBuffer->Barrier(pipelineBarrier, bufferMemoryBarrier);

		// ----------------------------------------------------------------------------------------------------

		*ppDstBuffer = staging.pBuffer;
		*pDstBufferAllocation = staging.bufferAllocation;

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	ImmediateContext* ImmediateContext::Create(IV3DDevice* pV3DDevice, IV3DQueue* pQueue, ResourceMemoryManager* pResourceMemoryManager)
	{
		ImmediateContext* pImmediateContext = VE_NEW_T(ImmediateContext);
		if (pImmediateContext == nullptr)
		{
			return nullptr;
		}

		if (pImmediateContext->Initialize(pV3DDevice, pQueue, pResourceMemoryManager) == false)
		{
			VE_DELETE_T(pImmediateContext, ImmediateContext);
		}

		return pImmediateContext;
	}

	void ImmediateContext::Destroy()
	{
		VE_DELETE_THIS_T(this, ImmediateContext);
	}

	ImmediateContext::ImmediateContext() :
		m_pV3DDevice(nullptr),
		m_pResourceMemoryManager(nullptr),
		m_pCommandBuffer(nullptr),
		m_pFence(nullptr),
		m_pQueue(nullptr)
	{
	}

	ImmediateContext::~ImmediateContext()
	{
		VE_RELEASE(m_pFence);
		VE_RELEASE(m_pCommandBuffer);
		VE_RELEASE(m_pQueue);
		VE_RELEASE(m_pV3DDevice);
	}

	bool ImmediateContext::Initialize(IV3DDevice* pV3DDevice, IV3DQueue* pQueue, ResourceMemoryManager* pResourceMemoryManager)
	{
		pV3DDevice->AddRef();
		m_pV3DDevice = pV3DDevice;

		m_pResourceMemoryManager = pResourceMemoryManager;

		pQueue->AddRef();
		m_pQueue = pQueue;

		V3DCommandPoolDesc commandPoolDesc;
		commandPoolDesc.queueFamily = pQueue->GetFamily();
		commandPoolDesc.usageFlags = V3D_COMMAND_POOL_USAGE_RESET_COMMAND_BUFFER;
		if (m_pV3DDevice->CreateCommandBuffer(commandPoolDesc, V3D_COMMAND_BUFFER_TYPE_PRIMARY, &m_pCommandBuffer, VE_INTERFACE_DEBUG_NAME(L"VE_ImmediateContext")) != V3D_OK)
		{
			return false;
		}

		if (m_pV3DDevice->CreateFence(false, &m_pFence, L"VE_ImmediateContext") != V3D_OK)
		{
			return false;
		}

		return true;
	}

}