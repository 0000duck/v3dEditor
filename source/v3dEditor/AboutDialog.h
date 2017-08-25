#pragma once

#include "GuiPopup.h"

namespace ve {

	class AboutDialog final : public GuiPopup
	{
	public:
		AboutDialog();
		virtual~AboutDialog();

	private:
		bool OnRender() override;
	};

}