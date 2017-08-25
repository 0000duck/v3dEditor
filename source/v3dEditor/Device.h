#pragma once

namespace ve {

	class ImmediateContext;
	class ResourceMemoryManager;
	class SamplerFactory;
	class TextureManager;

	class Device final
	{
	public:
		static DevicePtr Create(LoggerPtr logger);

		Device();
		~Device();

		void Dispose();

		void DumpResourceMemory();

		VE_DECLARE_ALLOCATOR

	private:
		struct Impl;
		Impl* impl;

		IV3DDevice* GetNativeDevicePtr();
		IV3DQueue* GetNativeGraphicsQueuePtr();
		IV3DQueue* GetNativepImmediateQueuePtr();

		ResourceMemoryManager* GetResourceMemoryManagerPtr();
		SamplerFactory* GetSamplerFactoryPtr();
		TextureManager* GetTextureManagerPtr();
		ImmediateContext* GetImmediateContextPtr();

		void AddDeviceContext(DeviceContext* pDeviceContext);
		void RemoveDeviceContext(DeviceContext* pDeviceContext);
		void WaitDeviceContexts();

		friend class ResourceMemoryManager;
		friend class ImmediateContext;
		friend class DeviceContext;
	};

}