#include "FpsDialog.h"

namespace ve {

	FpsDialog::FpsDialog() : GuiFloat("Fps", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings),
		m_ScreenSize(0),
		m_Fps(0.0),
		m_ElapsedTime(0.0)
	{
	}

	FpsDialog::~FpsDialog()
	{
	}

	void FpsDialog::Render(const glm::uvec2& screenSize, float fps, float elapsedTime)
	{
		m_ScreenSize = screenSize;
		m_Fps = fps;
		m_ElapsedTime = elapsedTime;

		GuiFloat::Render();
	}

	bool FpsDialog::OnRender()
	{
		ImGui::Text("Fps: %.4f", m_Fps);
		ImGui::Text("ElapsedTime: %.4f ms", m_ElapsedTime);

		ImVec2 windowSize = ImGui::GetWindowSize();

		ImVec2 windowPos;
		windowPos.x = 16.0f;
		windowPos.y = static_cast<float>(m_ScreenSize.y) - windowSize.y - 16.0f;

		ImGui::SetWindowPos(GetName(), windowPos);

		return false;
	}

}