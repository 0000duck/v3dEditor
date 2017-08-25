#include "DeviceContext.h"
#include "Device.h"
#include "IDeviceContextListener.h"
#include "ImmediateContext.h"
#include "UpdatingQueue.h"
#include "DeletingQueue.h"
#include "GraphicsFactory.h"

#define VE_DC_IMPL_CRITICAL(func, result) \
	{ \
		if (func != result) \
		{ \
			pListener->OnCriticalError(); \
			return; \
		} \
	}

#define VE_DC_CRITICAL(func, result) \
	{ \
		if (func != result) \
		{ \
			impl->pListener->OnCriticalError(); \
			return; \
		} \
	}

namespace ve {

	/****************************************/
	/* private - struct DeviceContext::Impl */
	/****************************************/

	struct DeviceContext::Impl
	{
		DevicePtr device;
		IV3DSwapChain* pV3DSwapChain;
		UpdatingQueue* pUpdatingQueue;
		DeletingQueue* pDeletingQueue;
		GraphicsFactory* pGraphicsFactory;
		IDeviceContextListener* pListener;
		collection::Vector<DeviceContext::Frame> frames;

		Impl() :
			pV3DSwapChain(nullptr),
			pUpdatingQueue(nullptr),
			pDeletingQueue(nullptr),
			pGraphicsFactory(nullptr),
			pListener(nullptr)
		{
		}

		bool Initialize(DeviceContext* pOwner, DevicePtr device, HWND windowHandle, uint32_t bufferCount, IDeviceContextListener* pListener)
		{
			this->device = device;
			this->pListener = pListener;

			IV3DDevice* pV3DDevice = device->GetNativeDevicePtr();
			IV3DQueue* pGraphicsQueue = device->GetNativeGraphicsQueuePtr();

			// ----------------------------------------------------------------------------------------------------
			// スワップチェインを作成
			// ----------------------------------------------------------------------------------------------------

			RECT clientRect;

			if (GetClientRect(windowHandle, &clientRect) == FALSE)
			{
				return false;
			}

			V3DSwapChainDesc swapChainDesc;
			swapChainDesc.windowHandle = windowHandle;
			swapChainDesc.imageFormat = V3D_FORMAT_UNDEFINED;
			swapChainDesc.imageWidth = clientRect.right - clientRect.left;
			swapChainDesc.imageHeight = clientRect.bottom - clientRect.top;
			swapChainDesc.imageCount = bufferCount;
			swapChainDesc.imageUsageFlags = V3D_IMAGE_USAGE_TRANSFER_DST | V3D_IMAGE_USAGE_COLOR_ATTACHMENT;
			swapChainDesc.queueFamily = pGraphicsQueue->GetFamily();
			swapChainDesc.queueWaitDstStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			swapChainDesc.fullscreenAssistEnable = true;
			swapChainDesc.vsyncEnable = false;
			swapChainDesc.windowed = true;

			V3DSwapChainCallbacks swapChainCallbacks;
			swapChainCallbacks.pLostFunction = Impl::LostSwapChain;
			swapChainCallbacks.pRestoreFunction = Impl::RestoreSwapChain;
			swapChainCallbacks.pUserData = this;

			VE_DEBUG_CODE(std::wstringstream swapChainName);
			VE_DEBUG_CODE(swapChainName << L"VE_SwapChain_" << reinterpret_cast<uint64_t>(windowHandle));

			V3D_RESULT result = pV3DDevice->CreateSwapChain(swapChainDesc, swapChainCallbacks, &pV3DSwapChain, VE_INTERFACE_DEBUG_NAME(swapChainName.str().c_str()));
			if (result != V3D_OK)
			{
				return false;
			}

			swapChainDesc = pV3DSwapChain->GetDesc();

			// ----------------------------------------------------------------------------------------------------
			// フレームを作成
			// ----------------------------------------------------------------------------------------------------

			if(CreateFrames(pV3DSwapChain->GetDesc().imageCount) == false)
			{
				return false;
			}

			// ----------------------------------------------------------------------------------------------------
			// 更新キューを作成
			// ----------------------------------------------------------------------------------------------------

			pUpdatingQueue = UpdatingQueue::Create(swapChainDesc.imageCount);
			if (pUpdatingQueue == nullptr)
			{
				return false;
			}

			// ----------------------------------------------------------------------------------------------------
			// 削除キューを作成
			// ----------------------------------------------------------------------------------------------------

			pDeletingQueue = DeletingQueue::Create(device->GetResourceMemoryManagerPtr(), swapChainDesc.imageCount);
			if (pDeletingQueue == nullptr)
			{
				return false;
			}

			// ----------------------------------------------------------------------------------------------------
			// グラフィックスファクトリー
			// ----------------------------------------------------------------------------------------------------

			pGraphicsFactory = GraphicsFactory::Create(pOwner);
			if (pGraphicsFactory == nullptr)
			{
				return false;
			}

			// ----------------------------------------------------------------------------------------------------
			// デバイスコンテキストを登録
			// ----------------------------------------------------------------------------------------------------

			device->AddDeviceContext(pOwner);

			// ----------------------------------------------------------------------------------------------------

			return true;
		}

		bool CreateFrames(uint32_t frameCount)
		{
			VE_ASSERT(pV3DSwapChain != nullptr);
			VE_ASSERT(frames.empty() == true);

			IV3DDevice* pV3DDevice = device->GetNativeDevicePtr();
			ImmediateContext* pImmediateContext = device->GetImmediateContextPtr();

			// コマンドプール
			V3DCommandPoolDesc commandPoolDesc;
			commandPoolDesc.queueFamily = device->GetNativeGraphicsQueuePtr()->GetFamily();
			commandPoolDesc.usageFlags = V3D_COMMAND_POOL_USAGE_RESET_COMMAND_BUFFER;

			// イメージビュー
			V3DImageViewDesc imageViewDesc;
			imageViewDesc.type = V3D_IMAGE_VIEW_TYPE_2D;
			imageViewDesc.baseLevel = 0;
			imageViewDesc.levelCount = 1;
			imageViewDesc.baseLayer = 0;
			imageViewDesc.layerCount = 1;

			// パイプラインバリア
			V3DPipelineBarrier pipelineBarrier;
			pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TOP_OF_PIPE;
			pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_BOTTOM_OF_PIPE;
			pipelineBarrier.dependencyFlags = 0;

			// イメージビューバリア
			V3DImageViewMemoryBarrier memoryBarrier;
			memoryBarrier.srcAccessMask = 0;
			memoryBarrier.dstAccessMask = V3D_ACCESS_MEMORY_READ;
			memoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
			memoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
			memoryBarrier.srcLayout = V3D_IMAGE_LAYOUT_UNDEFINED;
			memoryBarrier.dstLayout = V3D_IMAGE_LAYOUT_PRESENT_SRC;

			IV3DCommandBuffer* pCommandBuffer = pImmediateContext->Begin();

			for (uint32_t i = 0; i < frameCount; i++)
			{
				DeviceContext::Frame frame;
				VE_DEBUG_CODE(StringW debugStr);

				/************************************/
				/* グラフィックスコマンドバッファー */
				/************************************/

				VE_DEBUG_CODE(debugStr = L"VE_GraphicsCommandBuffer_" + std::to_wstring(i));
				V3D_RESULT result = pV3DDevice->CreateCommandBuffer(commandPoolDesc, V3D_COMMAND_BUFFER_TYPE_PRIMARY, &frame.pGraphicsCommandBuffer, VE_INTERFACE_DEBUG_NAME(debugStr.c_str()));
				if (result != V3D_OK)
				{
					pImmediateContext->End();
					return false;
				}

				/**************************/
				/* グラフィックスフェンス */
				/**************************/

				VE_DEBUG_CODE(debugStr = L"VE_GraphicsFence_" + std::to_wstring(i));
				result = pV3DDevice->CreateFence(true, &frame.pGraphicsFence, VE_INTERFACE_DEBUG_NAME(debugStr.c_str()));
				if (result != V3D_OK)
				{
					frame.pGraphicsCommandBuffer->Release();
					pImmediateContext->End();
					return false;
				}

				/******************/
				/* イメージビュー */
				/******************/

				IV3DImage* pImage;
				pV3DSwapChain->GetImage(i, &pImage);

				VE_DEBUG_CODE(debugStr = L"VE_BackBuffer_" + std::to_wstring(i));
				result = pV3DDevice->CreateImageView(pImage, imageViewDesc, &frame.pImageView, VE_INTERFACE_DEBUG_NAME(debugStr.c_str()));
				if (result != V3D_OK)
				{
					pImage->Release();
					frame.pGraphicsFence->Release();
					frame.pGraphicsCommandBuffer->Release();
					pImmediateContext->End();
					return false;
				}

				memoryBarrier.pImageView = frame.pImageView;
				pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

				pImage->Release();

				frames.push_back(frame);
			}

			pImmediateContext->End();

			return true;
		}

		void ReleaseFrames()
		{
			if (frames.empty() == false)
			{
				auto it_begin = frames.begin();
				auto it_end = frames.end();

				for (auto it = it_begin; it != it_end; ++it)
				{
					it->pGraphicsFence->Wait();

					it->pImageView->Release();
					it->pGraphicsFence->Release();
					it->pGraphicsCommandBuffer->Release();
				}

				frames.clear();
			}
		}

		void WaitIdle()
		{
			if (frames.empty() == false)
			{
				auto it_begin = frames.begin();
				auto it_end = frames.end();

				for (auto it = it_begin; it != it_end; ++it)
				{
					it->pGraphicsFence->Wait();
				}
			}

			IV3DCommandBuffer* pCommandBuffer = device->GetImmediateContextPtr()->Begin();
			pUpdatingQueue->FlushAll(pCommandBuffer);
			device->GetImmediateContextPtr()->End();

			pDeletingQueue->FlushAll();
		}

		void OnLost()
		{
			device->WaitDeviceContexts();

			pListener->OnLost();
			pGraphicsFactory->Lost();
			ReleaseFrames();

			device->WaitDeviceContexts();
		}

		void OnRestore()
		{
			device->WaitDeviceContexts();

			const V3DSwapChainDesc& swapChainDesc = pV3DSwapChain->GetDesc();

			VE_DC_IMPL_CRITICAL(CreateFrames(swapChainDesc.imageCount), true);
			VE_DC_IMPL_CRITICAL(pGraphicsFactory->Restore(), true);

			pListener->OnRestore();
		}

		static void V3D_CALLBACK LostSwapChain(void* pUserData)
		{
			static_cast<DeviceContext::Impl*>(pUserData)->OnLost();
		}

		static void V3D_CALLBACK RestoreSwapChain(void* pUserData)
		{
			static_cast<DeviceContext::Impl*>(pUserData)->OnRestore();
		}

		VE_DECLARE_ALLOCATOR
	};

	/**************************/
	/* public - DeviceContext */
	/**************************/

	DeviceContextPtr DeviceContext::Create(DevicePtr device, HWND windowHandle, uint32_t bufferCount, IDeviceContextListener* pListener)
	{
		if ((device == nullptr) || (windowHandle == nullptr) || (pListener == nullptr))
		{
			return nullptr;
		}

		DeviceContextPtr deviceContext = std::make_shared<ve::DeviceContext>();

		if (deviceContext->impl->Initialize(deviceContext.get(), device, windowHandle, bufferCount, pListener) == false)
		{
			return nullptr;
		}


		return std::move(deviceContext);
	}

	DeviceContext::DeviceContext() :
		impl(VE_NEW_T(DeviceContext::Impl))
	{
	}

	DeviceContext::~DeviceContext()
	{
		Dispose();
	}

	glm::uvec2 DeviceContext::GetScreenSize()
	{
		const V3DSwapChainDesc& sdwapChainDesc = impl->pV3DSwapChain->GetDesc();
		return glm::uvec2(sdwapChainDesc.imageWidth, sdwapChainDesc.imageHeight);
	}

	void DeviceContext::BeginRender()
	{
		// 次のレンダリングイメージを獲得
		VE_DC_CRITICAL(impl->pV3DSwapChain->AcquireNextImage(), V3D_OK);

		// 更新キューのフレームを設定
		uint32_t currentFrameIndex = GetCurrentFrameIndex();
		impl->pUpdatingQueue->UpdateCurrentFrame(currentFrameIndex);

		// コマンドの記録を開始
		DeviceContext::Frame* pCurrentFrame = GetFramePtr(currentFrameIndex);
		VE_DC_CRITICAL(pCurrentFrame->pGraphicsCommandBuffer->Begin(), V3D_OK);

		// 更新キューをフラッシュ 2
		impl->pUpdatingQueue->Flush(pCurrentFrame->pGraphicsCommandBuffer, currentFrameIndex);
	}

	void DeviceContext::EndRender()
	{
		DeviceContext::Frame* pCurrentFrame = GetCurrentFramePtr();
		IV3DQueue* pGraphicsQueue = impl->device->GetNativeGraphicsQueuePtr();

		// コマンドの記録を終了
		VE_DC_CRITICAL(pCurrentFrame->pGraphicsCommandBuffer->End(), V3D_OK);

		// コマンドをキューに送信
		VE_DC_CRITICAL(pCurrentFrame->pGraphicsFence->Reset(), V3D_OK);
		VE_DC_CRITICAL(pGraphicsQueue->Submit(impl->pV3DSwapChain, 1, &pCurrentFrame->pGraphicsCommandBuffer, pCurrentFrame->pGraphicsFence), V3D_OK);

		// 次のフレームを待機
		VE_DC_CRITICAL(GetNextFramePtr()->pGraphicsFence->Wait(), V3D_OK);

		// プレゼント
		VE_DC_CRITICAL(pGraphicsQueue->Present(impl->pV3DSwapChain), V3D_OK);

		// 削除キューをフラッシュ
		impl->pDeletingQueue->Flush();
	}

	void DeviceContext::WaitIdle()
	{
		impl->WaitIdle();
	}

	void DeviceContext::Dispose()
	{
		if (impl != nullptr)
		{
			impl->WaitIdle();

			impl->device->RemoveDeviceContext(this);

			impl->ReleaseFrames();

			if (impl->pGraphicsFactory != nullptr)
			{
				impl->pGraphicsFactory->Destroy();
			}

			if (impl->pUpdatingQueue != nullptr)
			{
				impl->pUpdatingQueue->Destroy();
			}

			if (impl->pDeletingQueue != nullptr)
			{
				impl->pDeletingQueue->Destroy();
			}

			VE_RELEASE(impl->pV3DSwapChain);

//			impl->device->DumpResourceMemory();
			impl->device = nullptr;

			VE_DELETE_T(impl, Impl);
		}
	}

	DevicePtr DeviceContext::GetDevice()
	{
		return impl->device;
	}

	IV3DDevice* DeviceContext::GetNativeDevicePtr()
	{
		return impl->device->GetNativeDevicePtr();
	}

	const V3DSwapChainDesc& DeviceContext::GetNativeSwapchainDesc() const
	{
		return impl->pV3DSwapChain->GetDesc();
	}

	ResourceMemoryManager* DeviceContext::GetResourceMemoryManagerPtr()
	{
		return impl->device->GetResourceMemoryManagerPtr();
	}

	SamplerFactory* DeviceContext::GetSamplerFactoryPtr()
	{
		return impl->device->GetSamplerFactoryPtr();
	}

	TextureManager* DeviceContext::GetTextureManagerPtr()
	{
		return impl->device->GetTextureManagerPtr();
	}

	ImmediateContext* DeviceContext::GetImmediateContextPtr()
	{
		return impl->device->GetImmediateContextPtr();
	}

	GraphicsFactory* DeviceContext::GetGraphicsFactoryPtr()
	{
		return impl->pGraphicsFactory;
	}

	UpdatingQueue* DeviceContext::GetUpdatingQueuePtr()
	{
		return impl->pUpdatingQueue;
	}

	DeletingQueue* DeviceContext::GetDeletingQueuePtr()
	{
		return impl->pDeletingQueue;
	}

	uint32_t DeviceContext::GetFrameCount() const
	{
		return static_cast<uint32_t>(impl->frames.size());
	}

	uint32_t DeviceContext::GetCurrentFrameIndex() const
	{
		return impl->pV3DSwapChain->GetCurrentImageIndex();
	}

	uint32_t DeviceContext::GetNextFrameIndex() const
	{
		return (impl->pV3DSwapChain->GetCurrentImageIndex() + 1) % impl->pV3DSwapChain->GetDesc().imageCount;
	}

	DeviceContext::Frame* DeviceContext::GetFramePtr(uint32_t frameIndex)
	{
		return &impl->frames[frameIndex];
	}

	DeviceContext::Frame* DeviceContext::GetCurrentFramePtr()
	{
		return &impl->frames[impl->pV3DSwapChain->GetCurrentImageIndex()];
	}

	DeviceContext::Frame* DeviceContext::GetNextFramePtr()
	{
		uint32_t frameIndex = (impl->pV3DSwapChain->GetCurrentImageIndex() + 1) % impl->pV3DSwapChain->GetDesc().imageCount;
		return &impl->frames[frameIndex];
	}

	void DeviceContext::Lost()
	{
		impl->OnLost();
	}

	void DeviceContext::Restore()
	{
		impl->OnRestore();
	}

}