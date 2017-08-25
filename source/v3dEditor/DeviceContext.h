#pragma once

namespace ve {

	class IDeviceContextListener;
	class ResourceMemoryManager;
	class SamplerFactory;
	class TextureManager;
	class ImmediateContext;
	class GraphicsFactory;
	class UpdatingQueue;
	class DeletingQueue;

	class DeviceContext
	{
	public:
		static DeviceContextPtr Create(DevicePtr device, HWND windowHandle, uint32_t bufferCount, IDeviceContextListener* pListener);

		DeviceContext();
		~DeviceContext();

		glm::uvec2 GetScreenSize();

		void BeginRender();
		void EndRender();

		void WaitIdle();
		void Dispose();

		VE_DECLARE_ALLOCATOR

	private:
		struct Impl;
		Impl* impl;

		struct Frame
		{
			IV3DCommandBuffer* pGraphicsCommandBuffer;
			IV3DFence* pGraphicsFence;
			IV3DImageView* pImageView;
		};

		DevicePtr GetDevice();
		IV3DDevice* GetNativeDevicePtr();
		const V3DSwapChainDesc& GetNativeSwapchainDesc() const;

		ResourceMemoryManager* GetResourceMemoryManagerPtr();
		SamplerFactory* GetSamplerFactoryPtr();
		TextureManager* GetTextureManagerPtr();
		ImmediateContext* GetImmediateContextPtr();

		UpdatingQueue* GetUpdatingQueuePtr();
		DeletingQueue* GetDeletingQueuePtr();
		GraphicsFactory* GetGraphicsFactoryPtr();

		uint32_t GetFrameCount() const;
		uint32_t GetCurrentFrameIndex() const;
		uint32_t GetNextFrameIndex() const;
		DeviceContext::Frame* GetFramePtr(uint32_t frameIndex);
		DeviceContext::Frame* GetCurrentFramePtr();
		DeviceContext::Frame* GetNextFramePtr();

		void Lost();
		void Restore();

		friend class DynamicBuffer;
		friend class Texture;
		friend class Material;
		friend class SkeletalMesh;
		friend class SkeletalModel;
		friend class GraphicsFactory;
		friend class UpdatingQueue;
		friend class DeletingQueue;
		friend class Scene;
		friend class Gui;
	};

}