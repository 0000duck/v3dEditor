#pragma once

#include "GuiWindow.h"

namespace ve {

	class GuiPopup : public GuiWindow
	{
	public:
		GuiPopup(const char* pCaption, bool closeButtonEnable, ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders);
		GuiPopup(const char* pCaption, bool closeButtonEnable, const glm::ivec2& size, ImGuiWindowFlags flags = ImGuiWindowFlags_ShowBorders);
		GuiPopup(const char* pCaption, bool closeButtonEnable, const glm::ivec2& pos, const glm::ivec2& size, ImGuiWindowFlags flags = ImGuiWindowFlags_ShowBorders);
		virtual ~GuiPopup();

		bool IsShow() const final override;
		void Show() override;
		void ShowModal();

	protected:
		GuiWindow::STATE OnBegin() override;
		void OnEnd() override;
		void OnClose() override;

	private:
		enum TYPE
		{
			TYPE_SHOW = 0,
			TYPE_SHOW_MODAL = 1,
		};

		GuiPopup::TYPE m_Type;
		bool m_CloseButtonEnable;
		bool m_ReqestShow;
		bool m_Show;
	};

}