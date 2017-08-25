#pragma once

namespace ve {

	class LogControl final
	{
	public:
		LogControl();
		~LogControl();

		void Render(LoggerPtr logger);

	private:
		bool m_InfoEnable;
		bool m_ErrorEnable;
		bool m_WarningEnable;
		bool m_DebugEnable;
		bool m_AutoScrollEnable;
	};

}