#pragma once

namespace ve {

	class IDeviceContextListener
	{
	public:
		virtual ~IDeviceContextListener() {}

		virtual void OnLost() = 0;
		virtual void OnRestore() = 0;

		virtual void OnCriticalError() = 0;
	};

}