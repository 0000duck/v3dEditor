#include "GuiFloat.h"

namespace ve {

	GuiFloat::GuiFloat(const char* pCaption, ImGuiWindowFlags flags) : GuiWindow(pCaption, flags),
		m_Show(false)
	{
	}

	GuiFloat::GuiFloat(const char* pCaption, const glm::ivec2& size, ImGuiWindowFlags flags) : GuiWindow(pCaption, size, flags),
		m_Show(false)
	{
	}

	GuiFloat::GuiFloat(const char* pCaption, const glm::ivec2& pos, const glm::ivec2& size, ImGuiWindowFlags flags) : GuiWindow(pCaption, pos, size, flags),
		m_Show(false)
	{
	}

	GuiFloat::~GuiFloat()
	{
	}

	bool* GuiFloat::GetShowPtr()
	{
		return &m_Show;
	}

	bool GuiFloat::IsShow() const
	{
		return m_Show;
	}

	void GuiFloat::Show()
	{
		m_Show = true;
	}

	GuiWindow::STATE GuiFloat::OnBegin()
	{
		if (m_Show == false)
		{
			return GuiWindow::STATE_IDLING;
		}

		return (ImGui::Begin(GetName(), &m_Show, ImVec2(0, 0), 0.8f, GetInternalFlags()) == true)? GuiWindow::STATE_SHOW : GuiWindow::STATE_HIDE;
	}

	void GuiFloat::OnEnd()
	{
		ImGui::End();
	}

	void GuiFloat::OnClose()
	{
		m_Show = false;
	}
}