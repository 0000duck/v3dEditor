#include "LogControl.h"
#include "Logger.h"

namespace ve {

	LogControl::LogControl() :
		m_InfoEnable(true),
		m_ErrorEnable(true),
		m_WarningEnable(true),
		m_DebugEnable(true),
		m_AutoScrollEnable(true)
	{
	}

	LogControl::~LogControl()
	{
	}

	void LogControl::Render(LoggerPtr logger)
	{
		if (ImGui::Button("Clear###LogControl_Clear") == true)
		{
			logger->ClearItems();
		}

		ImGui::SameLine();
		ImGui::Checkbox("Info###LogControl_Info", &m_InfoEnable);
		ImGui::SameLine();
		ImGui::Checkbox("Error###LogControl_Error", &m_ErrorEnable);
		ImGui::SameLine();
		ImGui::Checkbox("Warning###LogControl_Warning", &m_WarningEnable);
		ImGui::SameLine();
		ImGui::Checkbox("Debug###LogControl_Debug", &m_DebugEnable);
		ImGui::SameLine();
		ImGui::Checkbox("AutoScroll###LogControl_AutoScroll", &m_AutoScrollEnable);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::BeginChild("##LogControl_ItemList", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);

		char log[4096]{};
		size_t itemCount;

		if (logger->BeginItem(itemCount) == true)
		{
			for (size_t i = 0; i < itemCount; i++)
			{
				const Logger::Item& item = logger->GetItem(i);

				log[0] = '\0';

				switch (item.type)
				{
				case Logger::TYPE_INFO:
					if (m_InfoEnable == true)
					{
						sprintf_s(log, "(info)    : %s", item.message.c_str());
					}
					break;
				case Logger::TYPE_ERROR:
					if (m_ErrorEnable == true)
					{
						sprintf_s(log, "(error)   : %s", item.message.c_str());
					}
					break;
				case Logger::TYPE_WARNING:
					if (m_WarningEnable == true)
					{
						sprintf_s(log, "(warning) : %s", item.message.c_str());
					}
					break;
				case Logger::TYPE_DEBUG:
					if (m_DebugEnable == true)
					{
						sprintf_s(log, "(debug)   : %s", item.message.c_str());
					}
					break;
				}

				if (log[0] != '\0')
				{
					ImGui::TextUnformatted(log);
				}
			}
		}

		logger->EndItem();

		if (m_AutoScrollEnable == true)
		{
			ImGui::SetScrollHere();
		}

		ImGui::EndChild();
	}

}