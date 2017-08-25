#include "BackgroundJobDialog.h"
#include "Logger.h"
#include "BackgroundQueue.h"

namespace ve {

	BackgroundJobDialog::BackgroundJobDialog() : GuiPopup("BackgroundJob", false, glm::ivec2(768, 512))
	{
	}

	BackgroundJobDialog::~BackgroundJobDialog()
	{
	}

	void BackgroundJobDialog::Dispose()
	{
		m_Logger = nullptr;
	}

	void BackgroundJobDialog::Initialize(LoggerPtr logger)
	{
//		m_Logger = Logger::Create();
		m_Logger = Logger::Create(logger);
	}

	LoggerPtr BackgroundJobDialog::GetLogger()
	{
		return m_Logger;
	}

	void BackgroundJobDialog::ShowModel(BackgroundJobHandlePtr handle)
	{
		VE_ASSERT(m_Handle == nullptr);

		m_Handle = handle;

		GuiPopup::ShowModal();
	}

	bool BackgroundJobDialog::OnRender()
	{
		VE_ASSERT(m_Handle != nullptr);

		// ----------------------------------------------------------------------------------------------------
		// Log
		// ----------------------------------------------------------------------------------------------------

		m_Control.Render(m_Logger);

		// ----------------------------------------------------------------------------------------------------
		// Close
		// ----------------------------------------------------------------------------------------------------

		SetResult(0);

		if (m_Handle->IsFinished() == true)
		{
			if (ImGui::Button("Close###BackgroundJob_Close") == true)
			{
				SetResult(m_Handle->GetState());
				m_Handle = nullptr;
				return true;
			}
		}
		else
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
			ImGui::Button("Close###BackgroundJob_Close");
			ImGui::PopStyleVar();
		}

		return false;
	}

}