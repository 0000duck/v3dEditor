#pragma once

namespace ve {

	class DynamicBuffer;

	class Gui final
	{
	public:
		static GuiPtr Create(DeviceContextPtr deviceContext, const char* pFontName, uint32_t fontSize);

		Gui();
		~Gui();

		void Dispose();

		void Begin(double deltaTime);
		void End();

		void Render();

		void WndProc(HWND windowHandle, UINT message, WPARAM wparam, LPARAM lparam);

		VE_DECLARE_ALLOCATOR

	private:
		static constexpr uint64_t VertexBufferResizeStep = sizeof(ImDrawVert) * 1024;
		static constexpr uint64_t IndexBufferResizeStep = sizeof(ImDrawIdx) * 1024;

		DeviceContextPtr m_DeviceContext;

		IV3DImageView* m_pFontImageView;
		ResourceAllocation m_FontImageMemoryHandle;
		IV3DDescriptorSet* m_pDescriptorSet;

		FrameBufferHandlePtr m_FrameBufferHandle;
		PipelineHandlePtr m_PipelineHandle;

		DynamicBuffer* m_pVertexBuffer;
		DynamicBuffer* m_pIndexBuffer;

		ImDrawData* m_pDrawData;

		bool Initialize(DeviceContextPtr deviceContext, const char* pFontName, uint32_t fontSize);

		void UpdateIMEPosition(int32_t x, int32_t y);
		void Build(ImDrawData* pDrawData);

		static void* ImGui_MemAlloc(size_t size);
		static void ImGui_MemFree(void* ptr);

		static void ImGui_SetInputScreenPosFn(int32_t x, int32_t y);
		static void ImGui_RenderDrawLists(ImDrawData* draw_data);
	};

}