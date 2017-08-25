#pragma once

#include "GuiPopup.h"
#include "LogControl.h"

namespace ve {

	class BackgroundJobDialog final : public GuiPopup
	{
	public:
		BackgroundJobDialog();
		virtual~BackgroundJobDialog();

		void Initialize(LoggerPtr parentLogger);

		LoggerPtr GetLogger();

		void Dispose();

		/************/
		/* GuiPopup */
		/************/

		void ShowModel(BackgroundJobHandlePtr handle);

	private:
		LoggerPtr m_Logger;
		LogControl m_Control;
		BackgroundJobHandlePtr m_Handle;

		bool OnRender() override;
	};

}