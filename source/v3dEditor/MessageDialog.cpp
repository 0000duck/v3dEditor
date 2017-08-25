#include "MessageDialog.h"

namespace ve {

	MessageDialog::MessageDialog() : GuiPopup("", false),
		m_Type(MessageDialog::TYPE_OK),
		m_UserIndex(0)
	{
	}

	MessageDialog::~MessageDialog()
	{
	}

	MessageDialog::TYPE MessageDialog::GetType() const
	{
		return m_Type;
	}

	void MessageDialog::SetType(MessageDialog::TYPE type)
	{
		m_Type = type;
	}

	const char* MessageDialog::GetText() const
	{
		return m_Text.c_str();
	}

	void MessageDialog::SetText(const char* pText)
	{
		m_Text = pText;
	}

	int32_t MessageDialog::GetUserIndex() const
	{
		return m_UserIndex;
	}

	void MessageDialog::SetUserIndex(int32_t index)
	{
		m_UserIndex = index;
	}

	bool MessageDialog::OnRender()
	{
		bool result = false;

		SetResult(MessageDialog::RESULT_CANCEL);

		ImGui::Text(m_Text.c_str());

		if (m_Type == TYPE_OK)
		{
			if (ImGui::Button("Ok###MessageDialog_Ok") == true)
			{
				SetResult(MessageDialog::RESULT_OK);
				result = true;
			}
		}
		else if (m_Type == TYPE_OK_CANCEL)
		{
			if (ImGui::Button("Ok###MessageDialog_Ok") == true)
			{
				SetResult(MessageDialog::RESULT_OK);
				result = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel###MessageDialog_Cancel") == true)
			{
				SetResult(MessageDialog::RESULT_CANCEL);
				result = true;
			}
		}
		else if (m_Type == TYPE_RETRY_CANCEL)
		{
			if (ImGui::Button("Retry###MessageDialog_Retry") == true)
			{
				SetResult(MessageDialog::RESULT_RETRY);
				result = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel###MessageDialog_Cancel") == true)
			{
				SetResult(MessageDialog::RESULT_CANCEL);
				result = true;
			}
		}
		else if (m_Type == TYPE_YES_NO)
		{
			if (ImGui::Button("Yes###MessageDialog_Yes") == true)
			{
				SetResult(MessageDialog::RESULT_YES);
				result = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("No###MessageDialog_No") == true)
			{
				SetResult(MessageDialog::RESULT_NO);
				result = true;
			}
		}
		else if (m_Type == TYPE_YES_NO_CANCEL)
		{
			if (ImGui::Button("Yes###MessageDialog_Yes") == true)
			{
				SetResult(MessageDialog::RESULT_YES);
				result = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("No###MessageDialog_No") == true)
			{
				SetResult(MessageDialog::RESULT_NO);
				result = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel###MessageDialog_Cancel") == true)
			{
				SetResult(MessageDialog::RESULT_CANCEL);
				result = true;
			}
		}

		return result;
	}

}