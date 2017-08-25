#pragma once

#include "GuiFloat.h"
#include "LogControl.h"

namespace ve {

	class LogDialog final : public GuiFloat
	{
	public:
		LogDialog();
		virtual~LogDialog();

		void SetDevice(DevicePtr device);
		void SetLogger(LoggerPtr logger);

		void Dispose();

	private:
		DevicePtr m_Device;
		LoggerPtr m_Logger;
		LogControl m_Control;

		bool OnRender() override;
	};

}