#include "Gui.h"
#include "Device.h"
#include "DeviceContext.h"
#include "ResourceMemoryManager.h"
#include "SamplerFactory.h"
#include "GraphicsFactory.h"
#include "ImmediateContext.h"
#include "DeletingQueue.h"
#include "DynamicBuffer.h"
#include <Shlobj.h>

static constexpr ImWchar VE_GUI_Glyph_Ranges[] =
{
	#include "resource\gui_glyph_ranges.inc"
};

namespace ve {

	GuiPtr Gui::Create(DeviceContextPtr deviceContext, const char* pFontName, uint32_t fontSize)
	{
		GuiPtr gui = std::make_shared<Gui>();

		if (gui->Initialize(deviceContext, pFontName, fontSize) == false)
		{
			return nullptr;
		}

		return gui;
	}

	Gui::Gui() :
		m_pFontImageView(nullptr),
		m_FontImageMemoryHandle(nullptr),
		m_pVertexBuffer(nullptr),
		m_pIndexBuffer(nullptr),
		m_pDrawData(nullptr)
	{
	}

	Gui::~Gui()
	{
		Dispose();
	}

	void Gui::Dispose()
	{
		if (m_DeviceContext != nullptr)
		{
			DeletingQueue* pDeletingQueue = m_DeviceContext->GetDeletingQueuePtr();

			if (m_pVertexBuffer != nullptr)
			{
				m_pVertexBuffer->Destroy();
				m_pVertexBuffer = nullptr;
			}

			if (m_pIndexBuffer != nullptr)
			{
				m_pIndexBuffer->Destroy();
				m_pIndexBuffer = nullptr;
			}

			DeleteDeviceChild(pDeletingQueue, &m_pDescriptorSet);

			DeleteResource(pDeletingQueue, &m_pFontImageView, &m_FontImageMemoryHandle);
		}

		m_DeviceContext = nullptr;

		ImGui::Shutdown();
	}

	void Gui::Begin(double deltaTime)
	{
		ImGuiIO& io = ImGui::GetIO();

		const V3DSwapChainDesc& swapChainDesc = m_DeviceContext->GetNativeSwapchainDesc();

		io.DisplaySize.x = static_cast<float>(swapChainDesc.imageWidth);
		io.DisplaySize.y = static_cast<float>(swapChainDesc.imageHeight);
		io.DisplayFramebufferScale.x = 1.0f;
		io.DisplayFramebufferScale.y = 1.0f;
		io.DeltaTime = static_cast<float>(deltaTime);

		ImGui::NewFrame();
	}

	void Gui::End()
	{
		ImGui::Render();
	}

	void Gui::Render()
	{
		if (m_pDrawData == nullptr)
		{
			IV3DCommandBuffer* pCommandBuffer = m_DeviceContext->GetCurrentFramePtr()->pGraphicsCommandBuffer;
			uint32_t frameIndex = m_DeviceContext->GetCurrentFrameIndex();

			pCommandBuffer->BeginRenderPass(m_FrameBufferHandle->GetPtr(frameIndex), true);
			pCommandBuffer->EndRenderPass();
			return;
		}

		// ----------------------------------------------------------------------------------------------------
		// コマンドを記録
		// ----------------------------------------------------------------------------------------------------

		uint32_t frameIndex = m_DeviceContext->GetCurrentFrameIndex();

		IV3DCommandBuffer* pCommandBuffer = m_DeviceContext->GetCurrentFramePtr()->pGraphicsCommandBuffer;
		const V3DSwapChainDesc& swapChainDesc = m_DeviceContext->GetNativeSwapchainDesc();

		GuiConstant constant;
		constant.scale.x = VE_FLOAT_DIV(2.0f, static_cast<float>(swapChainDesc.imageWidth));
		constant.scale.y = VE_FLOAT_DIV(2.0f, static_cast<float>(swapChainDesc.imageHeight));
		constant.translate.x = -1.0f;
		constant.translate.y = -1.0f;
		pCommandBuffer->PushConstant(m_PipelineHandle->GetPtr(), 0, &constant);

		pCommandBuffer->BeginRenderPass(m_FrameBufferHandle->GetPtr(frameIndex), true);

		pCommandBuffer->BindPipeline(m_PipelineHandle->GetPtr());
		pCommandBuffer->BindDescriptorSet(m_PipelineHandle->GetPtr(), 0, m_pDescriptorSet);
		pCommandBuffer->BindVertexBuffer(0, m_pVertexBuffer->GetNativeBufferPtr(), m_pVertexBuffer->GetNativeRangeSize() * frameIndex);
		pCommandBuffer->BindIndexBuffer(m_pIndexBuffer->GetNativeBufferPtr(), m_pIndexBuffer->GetNativeRangeSize() * frameIndex, V3D_INDEX_TYPE_UINT16);

		V3DViewport viewport{};
		viewport.rect.width = swapChainDesc.imageWidth;
		viewport.rect.height = swapChainDesc.imageHeight;
		viewport.maxDepth = 1.0f;
		pCommandBuffer->SetViewport(0, 1, &viewport);

		uint32_t firstIndex = 0;
		int32_t vertexOffset = 0;

		for (int32_t i = 0; i < m_pDrawData->CmdListsCount; i++)
		{
			const ImDrawList* pDrawList = m_pDrawData->CmdLists[i];

			for (int32_t j = 0; j < pDrawList->CmdBuffer.Size; j++)
			{
				const ImDrawCmd* pDrawCmd = &pDrawList->CmdBuffer[j];

				if (pDrawCmd->UserCallback)
				{
					pDrawCmd->UserCallback(pDrawList, pDrawCmd);
				}
				else
				{
					V3DRectangle2D scissor;
					scissor.x = (pDrawCmd->ClipRect.x > VE_FLOAT_EPSILON) ? static_cast<int32_t>(pDrawCmd->ClipRect.x) : 0;
					scissor.y = (pDrawCmd->ClipRect.y > VE_FLOAT_EPSILON) ? static_cast<int32_t>(pDrawCmd->ClipRect.y) : 0;
					scissor.width = static_cast<uint32_t>(pDrawCmd->ClipRect.z - pDrawCmd->ClipRect.x);
					scissor.height = static_cast<uint32_t>(pDrawCmd->ClipRect.w - pDrawCmd->ClipRect.y);

					pCommandBuffer->SetScissor(0, 1, &scissor);
					pCommandBuffer->DrawIndexed(pDrawCmd->ElemCount, 1, firstIndex, 0, vertexOffset);
				}

				firstIndex += pDrawCmd->ElemCount;
			}

			vertexOffset += pDrawList->VtxBuffer.Size;
		}

		pCommandBuffer->EndRenderPass();

		// ----------------------------------------------------------------------------------------------------

		m_pDrawData = nullptr;

		// ----------------------------------------------------------------------------------------------------
	}

	void Gui::WndProc(HWND windowHandle, UINT message, WPARAM wparam, LPARAM lparam)
	{
		ImGuiIO& io = ImGui::GetIO();

		switch (message)
		{
		case WM_MOUSEMOVE:
			io.MousePos.x = static_cast<float>(GET_X_LPARAM(lparam));
			io.MousePos.y = static_cast<float>(GET_Y_LPARAM(lparam));
			break;

		case WM_LBUTTONDOWN:
			io.MouseDown[0] = true;
			break;
		case WM_LBUTTONUP:
			io.MouseDown[0] = false;
			break;

		case WM_RBUTTONDOWN:
			io.MouseDown[1] = true;
			break;
		case WM_RBUTTONUP:
			io.MouseDown[1] = false;
			break;

		case WM_MBUTTONDOWN:
			io.MouseDown[2] = true;
			break;
		case WM_MBUTTONUP:
			io.MouseDown[2] = false;
			break;

		case WM_XBUTTONDOWN:
			if (wparam & XBUTTON1) { io.MouseDown[3] = true; }
			if (wparam & XBUTTON2) { io.MouseDown[4] = true; }
			break;
		case WM_XBUTTONUP:
			if (wparam & XBUTTON1) { io.MouseDown[3] = false; }
			if (wparam & XBUTTON2) { io.MouseDown[4] = false; }
			break;

		case WM_MOUSEWHEEL:
			io.MouseWheel = (GET_WHEEL_DELTA_WPARAM(wparam) > 0)? +1.0f : -1.0f;
			break;

		case WM_CHAR:
			if (wparam < 256)
			{
				io.AddInputCharacter(static_cast<ImWchar>(wparam));
			}
			break;

		case WM_IME_COMPOSITION:
			if (lparam & GCS_RESULTSTR)
			{
				HIMC imcHandle = ImmGetContext(windowHandle);
				LONG length = ImmGetCompositionString(imcHandle, GCS_RESULTSTR, nullptr, 0);
				collection::Vector<wchar_t> string;
				string.resize(length + 1);
				ImmGetCompositionString(imcHandle, GCS_RESULTSTR, string.data(), static_cast<DWORD>(string.size()));

				for (size_t i = 0; i < (string.size() - 1); i++)
				{
					io.AddInputCharacter(static_cast<ImWchar>(string[i]));
				}

				ImmReleaseContext(windowHandle, imcHandle);
			}
			break;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			io.KeysDown[wparam] = true;
			if (wparam == VK_CONTROL)
			{
				io.KeyCtrl = true;
			}
			else if (wparam == VK_MENU)
			{
				io.KeyAlt = true;
			}
			else if (wparam == VK_SHIFT)
			{
				io.KeyShift = true;
			}
			break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			io.KeysDown[wparam] = false;
			if (wparam == VK_CONTROL)
			{
				io.KeyCtrl = false;
			}
			else if (wparam == VK_MENU)
			{
				io.KeyAlt = false;
			}
			else if (wparam == VK_SHIFT)
			{
				io.KeyShift = false;
			}
			break;
		}
	}

	bool Gui::Initialize(DeviceContextPtr deviceContext, const char* pFontName, uint32_t fontSize)
	{
		m_DeviceContext = deviceContext;

		IV3DDevice* pV3DDevice = m_DeviceContext->GetNativeDevicePtr();

		// ----------------------------------------------------------------------------------------------------
		// ImGui の初期化
		// ----------------------------------------------------------------------------------------------------

		{
			ImGuiIO& io = ImGui::GetIO();

			io.MemAllocFn = Gui::ImGui_MemAlloc;
			io.MemFreeFn = Gui::ImGui_MemFree;

			io.UserData = this;
			io.RenderDrawListsFn = Gui::ImGui_RenderDrawLists;
			io.ImeSetInputScreenPosFn = Gui::ImGui_SetInputScreenPosFn;
			io.IniFilename = nullptr;

			io.KeyMap[ImGuiKey_Tab] = VK_TAB;
			io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
			io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
			io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
			io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
			io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
			io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
			io.KeyMap[ImGuiKey_Home] = VK_HOME;
			io.KeyMap[ImGuiKey_End] = VK_END;
			io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
			io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
			io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
			io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
			io.KeyMap[ImGuiKey_A] = 'A';
			io.KeyMap[ImGuiKey_C] = 'C';
			io.KeyMap[ImGuiKey_V] = 'V';
			io.KeyMap[ImGuiKey_X] = 'X';
			io.KeyMap[ImGuiKey_Y] = 'Y';
			io.KeyMap[ImGuiKey_Z] = 'Z';

			/**************************/
			/* フォントイメージを作成 */
			/**************************/
		
			char fontDirPath[2014] = "";
			LPITEMIDLIST pIdl;

			if (::SHGetSpecialFolderLocation(nullptr, CSIDL_FONTS, &pIdl) == NOERROR)
			{
				SHGetPathFromIDListA(pIdl, fontDirPath);
				CoTaskMemFree(pIdl);
			}

			size_t fontDirPathLength = strlen(fontDirPath);
			if (fontDirPathLength == 0)
			{
				return false;
			}

			char lastFontDirPathChar = fontDirPath[fontDirPathLength - 1];
			if ((lastFontDirPathChar == '\\') || (lastFontDirPathChar == '/'))
			{
				strcat_s(fontDirPath, pFontName);
			}
			else
			{
				strcat_s(fontDirPath, "\\");
				strcat_s(fontDirPath, pFontName);
			}

			uint8_t* pFontImageData;
			int fontImageWidth;
			int fontImageHeight;
			io.Fonts->AddFontFromFileTTF(fontDirPath, static_cast<float>(fontSize), nullptr, VE_GUI_Glyph_Ranges);
			io.Fonts->GetTexDataAsRGBA32(&pFontImageData, &fontImageWidth, &fontImageHeight);
			io.ImeWindowHandle = m_DeviceContext->GetNativeSwapchainDesc().windowHandle;

			V3DImageDesc imageDesc;
			imageDesc.type = V3D_IMAGE_TYPE_2D;
			imageDesc.format = V3D_FORMAT_R8G8B8A8_UNORM;
			imageDesc.width = fontImageWidth;
			imageDesc.height = fontImageHeight;
			imageDesc.depth = 1;
			imageDesc.levelCount = 1;
			imageDesc.layerCount = 1;
			imageDesc.samples = V3D_SAMPLE_COUNT_1;
			imageDesc.tiling = V3D_IMAGE_TILING_OPTIMAL;
			imageDesc.usageFlags = V3D_IMAGE_USAGE_TRANSFER_DST | V3D_IMAGE_USAGE_SAMPLED;

			V3DImageFormatDesc imageFormatDesc;

			if (pV3DDevice->GetImageFormatDesc(imageDesc.format, imageDesc.type, imageDesc.tiling, imageDesc.usageFlags, &imageFormatDesc) != V3D_OK)
			{
				return false;
			}

			if ((imageFormatDesc.maxWidth < imageDesc.width) ||
				(imageFormatDesc.maxHeight < imageDesc.width) ||
				((imageFormatDesc.sampleCountFlags & imageDesc.samples) != imageDesc.samples))
			{
				return false;
			}

			IV3DImage* pImage;
			if (pV3DDevice->CreateImage(imageDesc, V3D_IMAGE_LAYOUT_UNDEFINED, &pImage, VE_INTERFACE_DEBUG_NAME(L"VE_FontImage")) != V3D_OK)
			{
				return false;
			}

			ResourceMemoryManager* pResourceMemoryManager = m_DeviceContext->GetResourceMemoryManagerPtr();

			m_FontImageMemoryHandle = pResourceMemoryManager->Allocate(pImage, V3D_MEMORY_PROPERTY_DEVICE_LOCAL);
			if (m_FontImageMemoryHandle == nullptr)
			{
				pImage->Release();
				return false;
			}

			ImmediateContext* pImmediateContext = m_DeviceContext->GetImmediateContextPtr();
			IV3DCommandBuffer* pCommandBuffer = pImmediateContext->Begin();

			V3DPipelineBarrier pipelineBarrier{};

			V3DImageMemoryBarrier memoryBarrier{};
			memoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
			memoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
			memoryBarrier.pImage = pImage;
			memoryBarrier.levelCount = 1;
			memoryBarrier.layerCount = 1;

			pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TOP_OF_PIPE;
			pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_TRANSFER;
			memoryBarrier.srcAccessMask = 0;
			memoryBarrier.dstAccessMask = V3D_ACCESS_TRANSFER_WRITE;
			memoryBarrier.srcLayout = V3D_IMAGE_LAYOUT_UNDEFINED;
			memoryBarrier.dstLayout = V3D_IMAGE_LAYOUT_TRANSFER_DST;
			pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

			V3DCopyBufferToImageRange range{};
			range.dstImageSubresource.layerCount = 1;
			range.dstImageSize.width = imageDesc.width;
			range.dstImageSize.height = imageDesc.height;
			range.dstImageSize.depth = imageDesc.depth;
			pImmediateContext->Upload(pImage, pFontImageData, fontImageWidth * fontImageHeight * 4 * sizeof(char), 1, &range);

			pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TRANSFER;
			pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			memoryBarrier.srcAccessMask = V3D_ACCESS_TRANSFER_WRITE;
			memoryBarrier.dstAccessMask = V3D_ACCESS_SHADER_READ;
			memoryBarrier.srcLayout = V3D_IMAGE_LAYOUT_TRANSFER_DST;
			memoryBarrier.dstLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

			pImmediateContext->End();

			V3DImageViewDesc imageViewDesc;
			imageViewDesc.type = V3D_IMAGE_VIEW_TYPE_2D;
			imageViewDesc.baseLevel = 0;
			imageViewDesc.levelCount = 1;
			imageViewDesc.baseLayer = 0;
			imageViewDesc.layerCount = 1;

			if (pV3DDevice->CreateImageView(pImage, imageViewDesc, &m_pFontImageView, VE_INTERFACE_DEBUG_NAME(L"VE_FontImageView")) != V3D_OK)
			{
				pImage->Release();
				pResourceMemoryManager->Free(m_FontImageMemoryHandle);
				return false;
			}

			pImage->Release();
		}

		// ----------------------------------------------------------------------------------------------------
		// サンプラーを作成
		// ----------------------------------------------------------------------------------------------------

		IV3DSampler* pSampler = m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_LINEAR, V3D_ADDRESS_MODE_REPEAT);
		if (pSampler == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// デスクリプタセットを更新
		// ----------------------------------------------------------------------------------------------------

		m_pDescriptorSet = m_DeviceContext->GetGraphicsFactoryPtr()->CreateNativeDescriptorSet(GraphicsFactory::ST_GUI, GraphicsFactory::SST_GUI_COPY);
		if (m_pDescriptorSet == nullptr)
		{
			return false;
		}

		m_pDescriptorSet->SetImageViewAndSampler(0, m_pFontImageView, pSampler);
		m_pDescriptorSet->Update();

		pSampler->Release();

		// ----------------------------------------------------------------------------------------------------
		// フレームバッファーのハンドルを取得
		// ----------------------------------------------------------------------------------------------------

		m_FrameBufferHandle = m_DeviceContext->GetGraphicsFactoryPtr()->GetFrameBufferHandle(GraphicsFactory::ST_GUI);

		// ----------------------------------------------------------------------------------------------------
		// パイプラインのハンドルを取得
		// ----------------------------------------------------------------------------------------------------

		m_PipelineHandle = m_DeviceContext->GetGraphicsFactoryPtr()->GetPipelineHandle(GraphicsFactory::ST_GUI, GraphicsFactory::SST_GUI_COPY);

		// ----------------------------------------------------------------------------------------------------
		// バーテックスバッファーを作成
		// ----------------------------------------------------------------------------------------------------

		m_pVertexBuffer = DynamicBuffer::Create(m_DeviceContext, V3D_BUFFER_USAGE_VERTEX, Gui::VertexBufferResizeStep, V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_VERTEX_READ, VE_INTERFACE_DEBUG_NAME(L"VE_Gui_VertexBuffer"));
		if (m_pVertexBuffer == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// インデックスバッファーを作成
		// ----------------------------------------------------------------------------------------------------

		m_pIndexBuffer = DynamicBuffer::Create(m_DeviceContext, V3D_BUFFER_USAGE_INDEX, Gui::IndexBufferResizeStep, V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_INDEX_READ, VE_INTERFACE_DEBUG_NAME(L"VE_Gui_IndexBuffer"));
		if (m_pIndexBuffer == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	void Gui::UpdateIMEPosition(int32_t x, int32_t y)
	{
		ImGuiIO& io = ImGui::GetIO();

		if (io.ImeWindowHandle != nullptr)
		{
			HWND windowHandle = (HWND)io.ImeWindowHandle;
			HIMC imcHandle = ImmGetContext(windowHandle);

			if (imcHandle != nullptr)
			{
				COMPOSITIONFORM cf;
				cf.ptCurrentPos.x = x;
				cf.ptCurrentPos.y = y;
				cf.dwStyle = CFS_FORCE_POSITION;
				ImmSetCompositionWindow(imcHandle, &cf);

				ImmReleaseContext(windowHandle, imcHandle);
			}
		}
	}

	void Gui::Build(ImDrawData* pDrawData)
	{
		// ----------------------------------------------------------------------------------------------------
		// バーテックスバッファーを作成
		// ----------------------------------------------------------------------------------------------------

		size_t vertiesMemorySize = pDrawData->TotalVtxCount * sizeof(ImDrawVert);

		if (m_pVertexBuffer->GetNativeRangeSize() < vertiesMemorySize)
		{
			vertiesMemorySize = (vertiesMemorySize + Gui::VertexBufferResizeStep - 1) / Gui::VertexBufferResizeStep * Gui::VertexBufferResizeStep;

			DynamicBuffer* pNextVertexBuffer = DynamicBuffer::Create(m_DeviceContext, V3D_BUFFER_USAGE_VERTEX, vertiesMemorySize, V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_VERTEX_READ, VE_INTERFACE_DEBUG_NAME(L"VE_Gui_VertexBuffer"));
			if (pNextVertexBuffer != nullptr)
			{
				m_pVertexBuffer->Destroy();
				m_pVertexBuffer = pNextVertexBuffer;
			}
			else
			{
				return;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// インデックスバッファーを作成
		// ----------------------------------------------------------------------------------------------------

		size_t indicesMemorySize = pDrawData->TotalIdxCount * sizeof(ImDrawIdx);

		if (m_pIndexBuffer->GetNativeRangeSize() < indicesMemorySize)
		{
			indicesMemorySize = (indicesMemorySize + Gui::IndexBufferResizeStep - 1) / Gui::IndexBufferResizeStep * Gui::IndexBufferResizeStep;

			DynamicBuffer* pNextIndexBuffer = DynamicBuffer::Create(m_DeviceContext, V3D_BUFFER_USAGE_INDEX, indicesMemorySize, V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_INDEX_READ, VE_INTERFACE_DEBUG_NAME(L"VE_Gui_IndexBuffer"));
			if (pNextIndexBuffer != nullptr)
			{
				m_pIndexBuffer->Destroy();
				m_pIndexBuffer = pNextIndexBuffer;
			}
			else
			{
				return;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// メッシュを書き込む
		//  現在同時にマップできないのであとで修正
		// ----------------------------------------------------------------------------------------------------

#if 1 
		ImDrawVert* pVertxMemory = reinterpret_cast<ImDrawVert*>(m_pVertexBuffer->Map());
		VE_ASSERT(pVertxMemory != nullptr);

		for (int32_t i = 0; i < pDrawData->CmdListsCount; i++)
		{
			const ImDrawList* pDrawList = pDrawData->CmdLists[i];
			memcpy(pVertxMemory, pDrawList->VtxBuffer.Data, pDrawList->VtxBuffer.Size * sizeof(ImDrawVert));
			pVertxMemory += pDrawList->VtxBuffer.Size;
		}

		m_pVertexBuffer->Unmap();

		ImDrawIdx* pIndexMemory = reinterpret_cast<ImDrawIdx*>(m_pIndexBuffer->Map());
		VE_ASSERT(pIndexMemory != nullptr);

		for (int32_t i = 0; i < pDrawData->CmdListsCount; i++)
		{
			const ImDrawList* pDrawList = pDrawData->CmdLists[i];
			memcpy(pIndexMemory, pDrawList->IdxBuffer.Data, pDrawList->IdxBuffer.Size * sizeof(ImDrawIdx));
			pIndexMemory += pDrawList->IdxBuffer.Size;
		}

		m_pIndexBuffer->Unmap();
#else
		ImDrawVert* pVertxMemory = reinterpret_cast<ImDrawVert*>(m_pVertexBuffer->Map());
		VE_ASSERT(pVertxMemory != nullptr);
		ImDrawIdx* pIndexMemory = reinterpret_cast<ImDrawIdx*>(m_pIndexBuffer->Map());
		VE_ASSERT(pIndexMemory != nullptr);

		for (int32_t i = 0; i < pDrawData->CmdListsCount; i++)
		{
			const ImDrawList* pDrawList = pDrawData->CmdLists[i];

			memcpy(pVertxMemory, pDrawList->VtxBuffer.Data, pDrawList->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(pIndexMemory, pDrawList->IdxBuffer.Data, pDrawList->IdxBuffer.Size * sizeof(ImDrawIdx));

			pVertxMemory += pDrawList->VtxBuffer.Size;
			pIndexMemory += pDrawList->IdxBuffer.Size;
		}

		m_pIndexBuffer->Unmap();
		m_pVertexBuffer->Unmap();
#endif

		m_pDrawData = pDrawData;
	}

	void* Gui::ImGui_MemAlloc(size_t size)
	{
		return VE_MALLOC(size);
	}

	void Gui::ImGui_MemFree(void* ptr)
	{
		VE_FREE(ptr);
	}

	void Gui::ImGui_SetInputScreenPosFn(int32_t x, int32_t y)
	{
		static_cast<Gui*>(ImGui::GetIO().UserData)->UpdateIMEPosition(x, y);
	}

	void Gui::ImGui_RenderDrawLists(ImDrawData* draw_data)
	{
		static_cast<Gui*>(ImGui::GetIO().UserData)->Build(draw_data);
	}

}