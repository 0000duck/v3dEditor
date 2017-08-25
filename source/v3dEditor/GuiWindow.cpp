#include "GuiWindow.h"

namespace ve {

	GuiWindow::GuiWindow(const char* pHash, ImGuiWindowFlags flags) :
		m_Hash(pHash),
		m_Pos(glm::ivec2(0)),
		m_PosEnable(false),
		m_Size(glm::ivec2(0)),
		m_Flags(flags),
		m_Result(0)
	{
		SetCaption(pHash);
	}

	GuiWindow::GuiWindow(const char* pHash, const glm::ivec2& size, ImGuiWindowFlags flags) :
		m_Hash(pHash),
		m_Pos(glm::ivec2(0)),
		m_PosEnable(false),
		m_Size(size),
		m_Flags(flags),
		m_Result(0)
	{
		SetCaption(pHash);
	}

	GuiWindow::GuiWindow(const char* pHash, const glm::ivec2& pos, const glm::ivec2& size, ImGuiWindowFlags flags) :
		m_Hash(pHash),
		m_Pos(pos),
		m_PosEnable(true),
		m_Size(size),
		m_Flags(flags),
		m_Result(0)
	{
		SetCaption(pHash);
	}

	GuiWindow::~GuiWindow()
	{
	}

	const char* GuiWindow::GetName() const
	{
		return m_Name.c_str();
	}

	const char* GuiWindow::GetCaption() const
	{
		return m_Caption.c_str();
	}

	void GuiWindow::SetCaption(const char* pCaption)
	{
		m_Caption = pCaption;
		m_Name = m_Caption + "###" + m_Hash;
	}

	const char* GuiWindow::GetHash() const
	{
		return m_Hash.c_str();
	}

	void GuiWindow::SetHash(const char* pHash)
	{
		m_Hash = pHash;
		m_Name = m_Caption + "###" + m_Hash;
	}

	const glm::ivec2& GuiWindow::GetPosition() const
	{
		return m_Pos;
	}

	const glm::ivec2& GuiWindow::GetSize() const
	{
		return m_Size;
	}

	ImGuiWindowFlags GuiWindow::GetInternalFlags() const
	{
		return m_Flags;
	}

	int32_t GuiWindow::GetResult() const
	{
		return m_Result;
	}

	void GuiWindow::SetResult(int32_t result)
	{
		m_Result = result;
	}

	int32_t GuiWindow::Render()
	{
		int32_t result = 0;

		if (m_PosEnable == true)
		{
			ImGui::SetNextWindowPos(ImVec2(static_cast<float>(m_Pos.x), static_cast<float>(m_Pos.y)), ImGuiSetCond_Once);
		}

		if ((m_Flags & ImGuiWindowFlags_AlwaysAutoResize) == 0)
		{
			ImGui::SetNextWindowSize(ImVec2(static_cast<float>(m_Size.x), static_cast<float>(m_Size.y)), ImGuiSetCond_Once);
		}

		GuiWindow::STATE state = OnBegin();

		switch (state)
		{
		case GuiWindow::STATE_SHOW:
			if (OnRender() == true)
			{
				OnClose();

				result = m_Result;
			}

		case GuiWindow::STATE_HIDE:
			OnEnd();
			break;
		}

		return result;
	}
}