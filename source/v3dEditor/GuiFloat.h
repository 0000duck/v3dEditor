#pragma once

#include "GuiWindow.h"

namespace ve {

	class GuiFloat : public GuiWindow
	{
	public:
		GuiFloat(const char* pCaption, ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders);
		GuiFloat(const char* pCaption, const glm::ivec2& size, ImGuiWindowFlags flags = ImGuiWindowFlags_ShowBorders);
		GuiFloat(const char* pCaption, const glm::ivec2& pos, const glm::ivec2& size, ImGuiWindowFlags flags = ImGuiWindowFlags_ShowBorders);
		virtual ~GuiFloat();

		bool* GetShowPtr();
		bool IsShow() const final override;
		void Show() final override;

	private:
		bool m_Show;

		GuiWindow::STATE OnBegin() override;
		void OnEnd() override;
		void OnClose() override;
	};

}