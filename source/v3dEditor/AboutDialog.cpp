#include "AboutDialog.h"

namespace ve {

	AboutDialog::AboutDialog() : GuiPopup("About", false)
	{
	}

	AboutDialog::~AboutDialog()
	{
	}

	bool AboutDialog::OnRender()
	{
		ImGui::Text(VE_NAME_A);
		ImGui::SameLine();
		ImGui::Text(VE_VERSION_A);

		ImGui::Spacing();
		ImGui::Spacing();

		if (ImGui::Button("Ok###AboutDialog_Ok") == true)
		{
			return true;
		}

		return false;
	}

}