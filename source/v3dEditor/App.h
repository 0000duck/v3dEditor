#pragma once

#include "IDeviceContextListener.h"

#include "MessageDialog.h"
#include "BackgroundJobDialog.h"
#include "FileBrowser.h"
#include "ImportDialog.h"
#include "FpsDialog.h"
#include "OutlinerDialog.h"
#include "InspectorDialog.h"
#include "LogDialog.h"
#include "AboutDialog.h"

namespace ve {

	class App final : public IDeviceContextListener
	{
	public:
		App();
		~App();

		int32_t Run(HINSTANCE instanceHandle);

	private:
		enum STATE
		{
			STATE_OK = 0,
			STATE_SHUTDOWN = 1,
			STAGE_CRITICAL_ERROR = 2,
		};

		static constexpr long DefaultScreenWidth = 1024;
		static constexpr long DefaultScreenHeight = 768;
		const uint32_t WindowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE | WS_THICKFRAME;

		static constexpr long long BaseFps = 60;
		static constexpr uint32_t DefTimeSize = 2;
		static constexpr double InvDefTimeSize = 1.0 / static_cast<double>(DefTimeSize);

		enum MESSAGE_DIALOG_TYPE
		{
			MD_TYPE_DO_NOTHING = 0,
			MD_TYPE_QUIT = 1,
		};

		// ----------------------------------------------------------------------------------------------------

		struct CameraParam
		{
			float distanceCoiffe;
			float moveCoiffe;
			float rotationCoiffe;
		};

		// ----------------------------------------------------------------------------------------------------

		static App* s_pThis;

		HWND m_WindowHandle;

		LoggerPtr m_Logger;
		BackgroundQueuePtr m_BackgroundQueue;
		DevicePtr m_Device;
		DeviceContextPtr m_DeviceContext;
		ScenePtr m_Scene;
		NodePtr m_CameraNode;
		NodePtr m_LightNode;
		GuiPtr m_Gui;
		
		ProjectPtr m_Project;

		MessageDialog m_MessageDialog;
		BackgroundJobDialog m_BackgroundJobDialog;
		ImportDialog m_ImportDialog;
		FileBrowser m_OpenFileBrowser;
		FpsDialog m_FpsDialog;
		OutlinerDialog m_OutlinerDialog;
		InspectorDialog m_InspectorDialog;
		LogDialog m_LogDialog;
		AboutDialog m_AboutDialog;

		double m_Fps = 0.0;
		double m_AverageFpsPerSec = 0.0;
		double m_DeltaTime = 0.0;

		glm::ivec2 m_MousePos;
		App::CameraParam m_CameraParam;

		ModelPtr m_LoadingModel;

		App::STATE m_State;

		// ----------------------------------------------------------------------------------------------------

		bool OnInitialize(HINSTANCE instanceHandle);
		void OnFinalize();

		void OnIdle();

		void OnMouseButtonDown(uint32_t keyFlags, const glm::ivec2 pos);
		void OnMouseButtonUp(uint32_t keyFlags, const glm::ivec2 pos);
		void OnMouseMove(uint32_t keyFlags, const glm::ivec2 pos);
		void OnMouseWheel(uint32_t keyFlags, int16_t delta, const glm::ivec2 pos);

		void CloseProject();
		void Shutdown();

		static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wparam, LPARAM lparam);

		/**************************/
		/* IDeviceContextListener */
		/**************************/

		void OnLost() override;
		void OnRestore() override;
		void OnCriticalError() override;
	};

}