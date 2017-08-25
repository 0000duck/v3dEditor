#include "Device.h"
#include "ImmediateContext.h"
#include "ResourceMemoryManager.h"
#include "SamplerFactory.h"
#include "TextureManager.h"
#include "DeviceContext.h"

namespace ve {

	/****************/
	/* Device::Impl */
	/****************/

	struct Device::Impl
	{
		LoggerPtr logger;

		IV3DInstance* pV3DInstance;
		IV3DDevice* pV3DDevice;

		IV3DQueue* pGraphicsQueue;
		IV3DQueue* pImmediateQueue;

		ImmediateContext* pImmediateContext;
		ResourceMemoryManager* pResourceMemoryManager;
		SamplerFactory* pSamplerFactory;
		TextureManager* pTextureManager;

		collection::List<DeviceContext*> deviceContexts;

		Impl() :
			pV3DInstance(nullptr),
			pV3DDevice(nullptr),
			pGraphicsQueue(nullptr),
			pImmediateQueue(nullptr),
			pImmediateContext(nullptr),
			pResourceMemoryManager(nullptr),
			pSamplerFactory(nullptr),
			pTextureManager(nullptr)
		{
		}

		bool Initialize(Device* pOwner, LoggerPtr logger)
		{
			this->logger = logger;

			// ----------------------------------------------------------------------------------------------------
			// インスタンスを作成
			// ----------------------------------------------------------------------------------------------------

			V3DInstanceDesc instanceDesc{};

#ifdef _DEBUG
			instanceDesc.layer = V3D_LAYER_VALIDATION;
#else //_DEBUG
			instanceDesc.layer = V3D_LAYER_OPTIMAL;
#endif //_DEBUG

			instanceDesc.memory.pAllocateFunction = Impl::AllocateMemory;
			instanceDesc.memory.pReallocateFunction = Impl::ReallocateMemory;
			instanceDesc.memory.pFreeFunction = Impl::FreeMemory;

			instanceDesc.log.flags = V3D_LOG_WARNING | V3D_LOG_ERROR | V3D_LOG_DEBUG;

			V3D_RESULT result = V3DCreateInstance(instanceDesc, &pV3DInstance);
			if (result != V3D_OK)
			{
				return false;
			}

			// ----------------------------------------------------------------------------------------------------
			// デバイスを作成
			// ----------------------------------------------------------------------------------------------------

			IV3DAdapter* pV3DAdapter;

			if (pV3DInstance->GetAdapterCount() == 0)
			{
				return false;
			}

			pV3DInstance->GetAdapter(0, &pV3DAdapter);

			result = pV3DInstance->CreateDevice(pV3DAdapter, &pV3DDevice, L"VE_Device");
			if (result != V3D_OK)
			{
				VE_RELEASE(pV3DAdapter);
				return false;
			}

			VE_RELEASE(pV3DAdapter);

			// ----------------------------------------------------------------------------------------------------
			// キューを取得
			// ----------------------------------------------------------------------------------------------------

			uint32_t queueFamilyCount = pV3DDevice->GetQueueFamilyCount();
			uint32_t queueFamily = ~0U;

			for (uint32_t i = 0; (i < queueFamilyCount) && (queueFamily == ~0U); i++)
			{
				const V3DDeviceQueueFamilyDesc& desc = pV3DDevice->GetQueueFamilyDesc(i);
				if ((desc.queueFlags & V3D_QUEUE_GRAPHICS) && (desc.queueCount >= 2))
				{
					queueFamily = i;
				}
			}

			pV3DDevice->GetQueue(queueFamily, 0, &pGraphicsQueue);
			pV3DDevice->GetQueue(queueFamily, 1, &pImmediateQueue);

			// ----------------------------------------------------------------------------------------------------
			// リソースメモリーマネージャーを作成
			// ----------------------------------------------------------------------------------------------------

			pResourceMemoryManager = ResourceMemoryManager::Create(logger, pOwner);
			if (pResourceMemoryManager == nullptr)
			{
				return false;
			}

			// ----------------------------------------------------------------------------------------------------
			// サンプラーファクトリーを作成
			// ----------------------------------------------------------------------------------------------------

			pSamplerFactory = SamplerFactory::Create(pV3DDevice);
			if (pSamplerFactory == nullptr)
			{
				return false;
			}

			// ----------------------------------------------------------------------------------------------------
			// テクスチャマネージャーを作成
			// ----------------------------------------------------------------------------------------------------

			pTextureManager = TextureManager::Create();
			if (pTextureManager == nullptr)
			{
				return false;
			}

			// ----------------------------------------------------------------------------------------------------
			// イミディアットコンテキストを作成
			// ----------------------------------------------------------------------------------------------------

			pImmediateContext = ImmediateContext::Create(pV3DDevice, pImmediateQueue, pResourceMemoryManager);
			if (pImmediateContext == nullptr)
			{
				return false;
			}

			// ----------------------------------------------------------------------------------------------------

			return true;
		}

		static void* V3D_CALLBACK AllocateMemory(size_t size, size_t alignment, void* pUserData)
		{
			return VE_ALIGNED_MALLOC(alignment, size);
		}

		static void* V3D_CALLBACK ReallocateMemory(void* pMemory, size_t size, size_t alignment, void* pUserData)
		{
			return VE_ALIGNED_REALLOC(pMemory, alignment, size);
		}

		static void V3D_CALLBACK FreeMemory(void* pMemory, void* pUserData)
		{
			VE_FREE(pMemory);
		}

		VE_DECLARE_ALLOCATOR
	};

	/*******************/
	/* public - Device */
	/*******************/

	DevicePtr Device::Create(LoggerPtr logger)
	{
		DevicePtr devicePtr = std::make_shared<ve::Device>();

		if (devicePtr->impl->Initialize(devicePtr.get(), logger) == false)
		{
			return nullptr;
		}

		return std::move(devicePtr);
	}

	Device::Device() :
		impl(VE_NEW_T(Device::Impl))
	{
	}

	Device::~Device()
	{
		Dispose();
	}

	void Device::Dispose()
	{
		if (impl != nullptr)
		{
			if (impl->pImmediateContext != nullptr)
			{
				impl->pImmediateContext->Destroy();
			}

			if (impl->pTextureManager != nullptr)
			{
				impl->pTextureManager->Destroy();
			}

			if (impl->pSamplerFactory != nullptr)
			{
				impl->pSamplerFactory->Destroy();
			}

			if (impl->pResourceMemoryManager != nullptr)
			{
				impl->pResourceMemoryManager->Destroy();
			}

			VE_RELEASE(impl->pImmediateQueue);
			VE_RELEASE(impl->pGraphicsQueue);

			VE_RELEASE(impl->pV3DDevice);
			VE_RELEASE(impl->pV3DInstance);

			impl->logger = nullptr;

			VE_DELETE_T(impl, Impl);
		}
	}

	void Device::DumpResourceMemory()
	{
		if (impl != nullptr)
		{
			impl->pResourceMemoryManager->Dump();
		}
	}

	IV3DDevice* Device::GetNativeDevicePtr()
	{
		return impl->pV3DDevice;
	}

	IV3DQueue* Device::GetNativeGraphicsQueuePtr()
	{
		return impl->pGraphicsQueue;
	}

	IV3DQueue* Device::GetNativepImmediateQueuePtr()
	{
		return impl->pImmediateQueue;
	}

	ImmediateContext* Device::GetImmediateContextPtr()
	{
		return impl->pImmediateContext;
	}

	ResourceMemoryManager* Device::GetResourceMemoryManagerPtr()
	{
		return impl->pResourceMemoryManager;
	}

	SamplerFactory* Device::GetSamplerFactoryPtr()
	{
		return impl->pSamplerFactory;
	}

	TextureManager* Device::GetTextureManagerPtr()
	{
		return impl->pTextureManager;
	}

	void Device::AddDeviceContext(DeviceContext* pDeviceContext)
	{
		impl->deviceContexts.push_back(pDeviceContext);
	}

	void Device::RemoveDeviceContext(DeviceContext* pDeviceContext)
	{
		impl->deviceContexts.remove(pDeviceContext);
	}

	void Device::WaitDeviceContexts()
	{
		auto it_begin = impl->deviceContexts.begin();
		auto it_end = impl->deviceContexts.end();

		for (auto it = it_begin; it != it_end; ++it)
		{
			(*it)->WaitIdle();
		}
	}

}
