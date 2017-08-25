#include "DynamicBuffer.h"
#include "Device.h"
#include "DeviceContext.h"
#include "ResourceMemoryManager.h"
#include "ImmediateContext.h"
#include "UpdatingQueue.h"
#include "DeletingQueue.h"

namespace ve {

	DynamicBuffer* DynamicBuffer::Create(DeviceContextPtr deviceContext, V3DFlags usageFlags, uint64_t size, V3DFlags stageMask, V3DFlags accessMask, const wchar_t* pDebugName)
	{
		DynamicBuffer* pDynamicBuffer = VE_NEW_T(DynamicBuffer);
		if (pDynamicBuffer == nullptr)
		{
			return nullptr;
		}

		if (pDynamicBuffer->Initialize(deviceContext, usageFlags, size, stageMask, accessMask, pDebugName) == false)
		{
			VE_DELETE_T(pDynamicBuffer, DynamicBuffer);
			return nullptr;
		}

		return pDynamicBuffer;
	}

	void DynamicBuffer::Destroy()
	{
		VE_DELETE_THIS_T(this, DynamicBuffer);
	}

	DynamicBuffer::DynamicBuffer() :
		m_StageMask(0),
		m_AccessMask(0),
		m_CopySize(0),
		m_RangeSize(0),
		m_UpdateMask(0),
		m_pHostBuffer(nullptr),
		m_HostBufferAllocation(nullptr),
		m_pDeviceBuffer(nullptr),
		m_DeviceBufferAllocation(nullptr)
	{
	}

	DynamicBuffer::~DynamicBuffer()
	{
		if (m_pDeviceBuffer != nullptr)
		{
			m_DeviceContext->GetDeletingQueuePtr()->Add(m_pDeviceBuffer, m_DeviceBufferAllocation);
		}

		if (m_pHostBuffer != nullptr)
		{
			m_DeviceContext->GetDeletingQueuePtr()->Add(m_pHostBuffer, m_HostBufferAllocation);
		}
	}

	void* DynamicBuffer::Map()
	{
		void* pMemory;

		m_pHostBuffer->Map(0, m_CopySize, &pMemory);

		return pMemory;
	}

	void DynamicBuffer::Unmap()
	{
		m_pHostBuffer->Unmap();

		m_DeviceContext->GetUpdatingQueuePtr()->Add(m_UpdatingHandle, m_pDeviceBuffer, m_StageMask, m_AccessMask, m_RangeSize, m_pHostBuffer, m_CopySize);
	}

	IV3DBuffer* DynamicBuffer::GetNativeBufferPtr()
	{
		return m_pDeviceBuffer;
	}

	uint32_t DynamicBuffer::GetNativeRangeSize() const
	{
		return m_RangeSize;
	}

	bool DynamicBuffer::Initialize(DeviceContextPtr deviceContext, V3DFlags usageFlags, uint64_t size, V3DFlags stageMask, V3DFlags accessMask, const wchar_t* pDebugName)
	{
		m_DeviceContext = deviceContext;

		m_StageMask = stageMask;
		m_AccessMask = accessMask;

		m_UpdatingHandle = UpdatingHandle::Create();
		if (m_UpdatingHandle == nullptr)
		{
			return false;
		}

		IV3DDevice* pNativeDevice = m_DeviceContext->GetNativeDevicePtr();
		const V3DDeviceCaps& deviceCaps = pNativeDevice->GetCaps();

		uint64_t memoryAlignment = deviceCaps.minMemoryMapAlignment;
		if (usageFlags & (V3D_BUFFER_USAGE_UNIFORM_TEXEL | V3D_BUFFER_USAGE_STORAGE_TEXEL)) { memoryAlignment = std::max(memoryAlignment, deviceCaps.minTexelBufferOffsetAlignment); }
		if (usageFlags & V3D_BUFFER_USAGE_UNIFORM) { memoryAlignment = std::max(memoryAlignment, deviceCaps.minUniformBufferOffsetAlignment); }
		if (usageFlags & V3D_BUFFER_USAGE_STORAGE) { memoryAlignment = std::max(memoryAlignment, deviceCaps.minStorageBufferOffsetAlignment); }

		m_CopySize = size;
		m_RangeSize = static_cast<uint32_t>((size + memoryAlignment - 1) / memoryAlignment * memoryAlignment);

		// ----------------------------------------------------------------------------------------------------
		// バッファーを作成
		// ----------------------------------------------------------------------------------------------------

		ResourceMemoryManager* pResourceMemoryManager = m_DeviceContext->GetResourceMemoryManagerPtr();

		/**********/
		/* ホスト */
		/**********/

		V3DBufferDesc hostBufferDesc;
		hostBufferDesc.usageFlags = usageFlags | V3D_BUFFER_USAGE_TRANSFER_SRC;
		hostBufferDesc.size = m_RangeSize;

		V3D_RESULT result = pNativeDevice->CreateBuffer(hostBufferDesc, &m_pHostBuffer, VE_INTERFACE_DEBUG_NAME(pDebugName));
		if (result != V3D_OK)
		{
			return false;
		}

		V3DFlags memoryProperty = V3D_MEMORY_PROPERTY_HOST_VISIBLE | V3D_MEMORY_PROPERTY_HOST_COHERENT;
		if (pNativeDevice->CheckResourceMemoryProperty(memoryProperty, m_pHostBuffer) != V3D_OK)
		{
			memoryProperty = V3D_MEMORY_PROPERTY_HOST_VISIBLE;
		}

		m_HostBufferAllocation = pResourceMemoryManager->Allocate(m_pHostBuffer, memoryProperty);
		if (m_HostBufferAllocation == nullptr)
		{
			return false;
		}

		/************/
		/* デバイス */
		/************/

		V3DBufferDesc deviceBufferDesc;
		deviceBufferDesc.usageFlags = usageFlags | V3D_BUFFER_USAGE_TRANSFER_DST;
		deviceBufferDesc.size = m_RangeSize * m_DeviceContext->GetFrameCount();

		result = pNativeDevice->CreateBuffer(deviceBufferDesc, &m_pDeviceBuffer, VE_INTERFACE_DEBUG_NAME(pDebugName));
		if (result != V3D_OK)
		{
			return false;
		}
		
		m_DeviceBufferAllocation = pResourceMemoryManager->Allocate(m_pDeviceBuffer, V3D_MEMORY_PROPERTY_DEVICE_LOCAL);
		if (m_DeviceBufferAllocation == nullptr)
		{
			return false;
		}

		/**********/
		/* バリア */
		/**********/

		ImmediateContext* pImmediateContext = m_DeviceContext->GetImmediateContextPtr();

		IV3DCommandBuffer* pCommandBuffer = pImmediateContext->Begin();

		V3DPipelineBarrier pipelineBarrier;
		pipelineBarrier.dependencyFlags = 0;

		V3DBufferMemoryBarrier memoryBuffer;
		memoryBuffer.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		memoryBuffer.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TOP_OF_PIPE;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_HOST;
		memoryBuffer.srcAccessMask = 0;
		memoryBuffer.dstAccessMask = V3D_ACCESS_HOST_WRITE;
		memoryBuffer.pBuffer = m_pHostBuffer;
		memoryBuffer.offset = 0;
		memoryBuffer.size = hostBufferDesc.size;
		pCommandBuffer->Barrier(pipelineBarrier, memoryBuffer);

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TOP_OF_PIPE;
		pipelineBarrier.dstStageMask = m_StageMask;
		memoryBuffer.srcAccessMask = 0;
		memoryBuffer.dstAccessMask = m_AccessMask;
		memoryBuffer.pBuffer = m_pDeviceBuffer;
		memoryBuffer.offset = 0;
		memoryBuffer.size = deviceBufferDesc.size;
		pCommandBuffer->Barrier(pipelineBarrier, memoryBuffer);

		pImmediateContext->End();

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

}