#include "LogDialog.h"
#include "Device.h"
#include "Logger.h"

namespace ve {

	LogDialog::LogDialog() : GuiFloat("Log", glm::ivec2(512, 256), ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_MenuBar)
	{
	}

	LogDialog::~LogDialog()
	{
	}

	void LogDialog::Dispose()
	{
		m_Logger = nullptr;
		m_Device = nullptr;
	}

	void LogDialog::SetDevice(DevicePtr device)
	{
		m_Device = device;
	}

	void LogDialog::SetLogger(LoggerPtr logger)
	{
		m_Logger = logger;
	}

	bool LogDialog::OnRender()
	{
		if (ImGui::BeginMenuBar() == true)
		{
			if (ImGui::BeginMenu("Dump###LogDialog_Dump") == true)
			{
				if (ImGui::MenuItem("ResourceMemory###LogDialog_Dump_ResourceMemory") == true)
				{
					m_Device->DumpResourceMemory();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		m_Control.Render(m_Logger);

		return false;
	}

}