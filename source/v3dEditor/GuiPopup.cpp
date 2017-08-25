#include "GuiPopup.h"

namespace ve {

	GuiPopup::GuiPopup(const char* pCaption, bool closeButtonEnable, ImGuiWindowFlags flags) : GuiWindow(pCaption, flags),
		m_Type(GuiPopup::TYPE_SHOW),
		m_CloseButtonEnable(closeButtonEnable),
		m_ReqestShow(false),
		m_Show(false)
	{
	}

	GuiPopup::GuiPopup(const char* pCaption, bool closeButtonEnable, const glm::ivec2& size, ImGuiWindowFlags flags) : GuiWindow(pCaption, size, flags),
		m_Type(GuiPopup::TYPE_SHOW),
		m_CloseButtonEnable(closeButtonEnable),
		m_ReqestShow(false),
		m_Show(false)
	{
	}

	GuiPopup::GuiPopup(const char* pCaption, bool closeButtonEnable, const glm::ivec2& pos, const glm::ivec2& size, ImGuiWindowFlags flags) : GuiWindow(pCaption, pos, size, flags),
		m_Type(GuiPopup::TYPE_SHOW),
		m_CloseButtonEnable(closeButtonEnable),
		m_ReqestShow(false),
		m_Show(false)
	{
	}

	GuiPopup::~GuiPopup()
	{
	}

	void GuiPopup::Show()
	{
		if (m_Show == false)
		{
			m_Type = GuiPopup::TYPE_SHOW;
			m_ReqestShow = true;
		}
	}

	void GuiPopup::ShowModal()
	{
		if (m_Show == false)
		{
			m_Type = GuiPopup::TYPE_SHOW_MODAL;
			m_ReqestShow = true;
		}
	}

	bool GuiPopup::IsShow() const
	{
		return m_Show;
	}

	GuiWindow::STATE GuiPopup::OnBegin()
	{
		if (m_ReqestShow == true)
		{
			ImGui::OpenPopup(GetName());
			m_Show = true;

			m_ReqestShow = false;
		}

		switch (m_Type)
		{
		case GuiPopup::TYPE_SHOW:
			return (ImGui::BeginPopup(GetName()) == true)? GuiWindow::STATE_SHOW : GuiWindow::STATE_IDLING;
		case GuiPopup::TYPE_SHOW_MODAL:
			return (ImGui::BeginPopupModal(GetName(), (m_CloseButtonEnable == true)? &m_Show : nullptr, GetInternalFlags()) == true) ? GuiWindow::STATE_SHOW : GuiWindow::STATE_IDLING;
		}

		return GuiWindow::STATE_IDLING;
	}

	void GuiPopup::OnEnd()
	{
		ImGui::EndPopup();
	}

	void GuiPopup::OnClose()
	{
		ImGui::CloseCurrentPopup();
		m_Show = false;
	}

}