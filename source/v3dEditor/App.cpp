#include "App.h"
#include <chrono>
#include <thread>
#include "Logger.h"
#include "BackgroundQueue.h"
#include "Device.h"
#include "DeviceContext.h"
#include "Scene.h"
#include "Camera.h"
#include "Material.h"
#include "Texture.h"
#include "FbxModelSource.h"
#include "SkeletalModel.h"
#include "Node.h"
#include "Gui.h"
#include "Project.h"

namespace ve {

	App* App::s_pThis = nullptr;

	App::App() :
		m_WindowHandle(nullptr),
		m_State(App::STATE_OK)
	{
		s_pThis = this;

		m_CameraParam.rotationCoiffe = 0.5f;
		m_CameraParam.moveCoiffe = 0.05f;
		m_CameraParam.distanceCoiffe = 1.0f;

		m_OpenFileBrowser.SetCaption("Open File");
		m_OpenFileBrowser.SetHash("Main_OpenFile");
		m_OpenFileBrowser.SetMode(FileBrowser::MODE_OPEN);
		m_OpenFileBrowser.AddExtension("xml");
	}

	App::~App()
	{
		s_pThis = nullptr;
	}

	int32_t App::Run(HINSTANCE instanceHandle)
	{
		// ----------------------------------------------------------------------------------------------------
		// 初期化
		// ----------------------------------------------------------------------------------------------------

		/********************/
		/* ウィンドウを作成 */
		/********************/

		WNDCLASSEX wcex{};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = (CS_HREDRAW | CS_VREDRAW);
		wcex.lpfnWndProc = ve::App::WindowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = instanceHandle;
		wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = L"v3dEditor";

		if (::RegisterClassEx(&wcex) == 0)
		{
			return -1;
		}

		RECT rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = ve::App::DefaultScreenWidth;
		rect.bottom = ve::App::DefaultScreenHeight;
		::AdjustWindowRect(&rect, ve::App::WindowStyle, FALSE);

		m_WindowHandle = ::CreateWindowEx(
			0,
			L"v3dEditor",
			L"v3dEditor",
			ve::App::WindowStyle,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			rect.right - rect.left,
			rect.bottom - rect.top,
			HWND_DESKTOP,
			nullptr,
			instanceHandle,
			nullptr);

		if (m_WindowHandle == nullptr)
		{
			return -1;
		}

		::ShowWindow(m_WindowHandle, SW_SHOW);

		/******************/
		/* メンバの初期化 */
		/******************/

		POINT mousePos;
		if (GetCursorPos(&mousePos) == FALSE)
		{
			return -1;
		}

		if (ScreenToClient(m_WindowHandle, &mousePos) == FALSE)
		{
			return -1;
		}

		/********************/
		/* アプリケーション */
		/********************/

		if (OnInitialize(instanceHandle) == false)
		{
			OnFinalize();
			return -1;
		}

		// ----------------------------------------------------------------------------------------------------
		// メインループ
		// ----------------------------------------------------------------------------------------------------

		auto startClock = std::chrono::high_resolution_clock::now();
		auto averageStartClock = startClock;
		uint64_t avarageCount = 0;
		double averageFps = 0.0;
		uint32_t defTimeHead = 0;
		double defTimes[DefTimeSize] = {};
		double sumTimes = 0.0;
		long long sleepErrorCount = 0;
		long long period = 1000000 / ve::App::BaseFps;

		MSG msg{};

		do
		{
			/*******/
			/* FPS */
			/*******/

			auto curClock = std::chrono::high_resolution_clock::now();

			// スリープ
			long long defCount = std::chrono::duration_cast<std::chrono::duration<long long, std::chrono::microseconds::period>>(curClock - startClock).count();
			long long sleepCount = period - defCount + sleepErrorCount;
			if (sleepCount > 0)
			{
				auto sleepStartCount = std::chrono::high_resolution_clock::now();
				std::this_thread::sleep_for(std::chrono::microseconds(sleepCount));
				auto sleepEndCount = std::chrono::high_resolution_clock::now();

				// スリープの誤差 ( 指定したスリープ時間よりも長くスリープした場合、値はマイナスになる )
				sleepErrorCount = sleepCount - std::chrono::duration_cast<std::chrono::duration<long long, std::chrono::microseconds::period>>(sleepEndCount - sleepStartCount).count();
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::microseconds(0));
				sleepErrorCount = 0;
			}

			curClock = std::chrono::high_resolution_clock::now();

			// 更新
			double defTime = std::chrono::duration_cast<std::chrono::duration<double, std::chrono::microseconds::period>>(curClock - startClock).count();
			if (defTime > VE_DOUBLE_EPSILON)
			{
				defTimeHead = (defTimeHead + 1) % ve::App::DefTimeSize;

				uint32_t defTimeTail = (defTimeHead + ve::App::DefTimeSize - 1) % ve::App::DefTimeSize;
				defTimes[defTimeTail] = defTime;

				double aveDef = (sumTimes + defTime) * ve::App::InvDefTimeSize;
				if (aveDef > VE_DOUBLE_EPSILON)
				{
					m_Fps = 1000000.0 / aveDef;
				}

				sumTimes += defTime - defTimes[defTimeHead];
			}
			else
			{
				m_Fps = 0.0;
			}

			m_DeltaTime = (m_Fps > VE_DOUBLE_EPSILON) ? VE_DOUBLE_RECIPROCAL(m_Fps) : 1.0;

			curClock = std::chrono::high_resolution_clock::now();
			uint64_t averageDefTime = std::chrono::duration_cast<std::chrono::duration<uint64_t, std::chrono::milliseconds::period>>(curClock - averageStartClock).count();
			if (averageDefTime >= 1000)
			{
				if (avarageCount > 0)
				{
					m_AverageFpsPerSec = averageFps / avarageCount;
				}
				else
				{
					m_AverageFpsPerSec = 0;
				}

				averageStartClock = curClock;
				averageFps = 0.0f;
				avarageCount = 0;
			}
			else
			{
				averageFps += m_Fps;
				avarageCount++;
			}

			startClock = std::chrono::high_resolution_clock::now();

			/********************/
			/* メッセージポンプ */
			/********************/

			while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == TRUE)
			{
				if (GetMessage(&msg, NULL, 0, 0) == TRUE)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
				{
					m_State = App::STATE_SHUTDOWN;
					break;
				}
			}

			if (m_State == App::STATE_OK)
			{
				OnIdle();
			}

		} while (m_State == App::STATE_OK);

		// ----------------------------------------------------------------------------------------------------

		return 0;
	}

	bool App::OnInitialize(HINSTANCE instanceHandle)
	{
		// ----------------------------------------------------------------------------------------------------
		// ロガーを作成
		// ----------------------------------------------------------------------------------------------------

		m_Logger = ve::Logger::Create();
		if (m_Logger == nullptr)
		{
			return false;
		}

		m_BackgroundQueue = BackgroundQueue::Create();
		if (m_BackgroundQueue == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// Device を作成
		// ----------------------------------------------------------------------------------------------------

		m_Device = ve::Device::Create(m_Logger);
		if (m_Device == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// DeviceContext を作成
		// ----------------------------------------------------------------------------------------------------

		m_DeviceContext = ve::DeviceContext::Create(m_Device, m_WindowHandle, 2, this);
		if (m_DeviceContext == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// Scene を作成
		// ----------------------------------------------------------------------------------------------------

		m_Scene = ve::Scene::Create(m_DeviceContext);
		if (m_Scene == nullptr)
		{
			return false;
		}

		m_CameraNode = m_Scene->GetRootNode()->FindChild(L"Camera");
		m_LightNode = m_Scene->GetRootNode()->FindChild(L"Light");

		// ----------------------------------------------------------------------------------------------------
		// Gui を作成
		// ----------------------------------------------------------------------------------------------------

		m_Gui = ve::Gui::Create(m_DeviceContext, "msgothic.ttc", 14);
		if (m_Gui == nullptr)
		{
			return false;
		}

		m_BackgroundJobDialog.Initialize(m_Logger);
		m_FpsDialog.Show();
		m_OutlinerDialog.SetScene(m_Scene);
		m_InspectorDialog.SetDeviceContext(m_DeviceContext);
		m_InspectorDialog.SetScene(m_Scene);
		m_InspectorDialog.SetNode(m_OutlinerDialog.GetNode());
		m_LogDialog.SetDevice(m_Device);
		m_LogDialog.SetLogger(m_Logger);

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	void App::OnFinalize()
	{
		if (m_BackgroundQueue != nullptr)
		{
			m_BackgroundQueue->Terminate();
			m_BackgroundQueue = nullptr;
		}

		m_OutlinerDialog.Dispose();
		m_InspectorDialog.Dispose();
		m_LogDialog.Dispose();

		m_Project = nullptr;

		if (m_Gui != nullptr)
		{
			m_Gui->Dispose();
			m_Gui = nullptr;
		}

		m_CameraNode = nullptr;
		m_LightNode = nullptr;

		if (m_Scene != nullptr)
		{
			m_Scene->Dispose();
			m_Scene = nullptr;
		}

		if (m_DeviceContext != nullptr)
		{
			m_DeviceContext->Dispose();
			m_DeviceContext = nullptr;
		}

		if (m_Device != nullptr)
		{
			m_Device->Dispose();
			m_Device = nullptr;
		}
	}

	void App::OnIdle()
	{
		// ----------------------------------------------------------------------------------------------------
		// 更新
		// ----------------------------------------------------------------------------------------------------

		/*******/
		/* GUI */
		/*******/

		m_Gui->Begin(m_DeltaTime);

		// Main menu
		if (ImGui::BeginMainMenuBar() == true)
		{
			if (ImGui::BeginMenu("File###MainMenu_File"))
			{
				if (ImGui::MenuItem("New...###MainMenu_File_New", nullptr, nullptr) == true)
				{
					m_ImportDialog.ShowModal();
				}

				if (ImGui::MenuItem("Open...###MainMenu_File_Open", nullptr, nullptr) == true)
				{
					m_OpenFileBrowser.ShowModal();
				}

				if (m_Project != nullptr)
				{
					if (ImGui::MenuItem("Save###MainMenu_File_Save", nullptr, nullptr) == true)
					{
						if (m_Project->Save(m_Logger, m_Scene) == false)
						{
							m_MessageDialog.SetCaption(VE_NAME_A);
							m_MessageDialog.SetType(MessageDialog::TYPE_OK);
							m_MessageDialog.SetText("Failed to save!");
							m_MessageDialog.SetUserIndex(App::MD_TYPE_DO_NOTHING);
							m_MessageDialog.ShowModal();
						}
					}
				}
				else
				{
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
					ImGui::MenuItem("Save###MainMenu_File_Save", nullptr, nullptr);
					ImGui::PopStyleVar();
				}

				if (m_Project != nullptr)
				{
					if (ImGui::MenuItem("Close###MainMenu_File_Close", nullptr, nullptr) == true)
					{
						CloseProject();
					}
				}
				else
				{
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
					ImGui::MenuItem("Close###MainMenu_File_Close", nullptr, nullptr);
					ImGui::PopStyleVar();
				}

				if (ImGui::MenuItem("Quit###MainMenu_File_Quit", nullptr, nullptr) == true)
				{
					m_MessageDialog.SetCaption(VE_NAME_A);
					m_MessageDialog.SetText("Are you sure you want to quit?");
					m_MessageDialog.SetType(MessageDialog::TYPE_YES_NO);
					m_MessageDialog.SetUserIndex(App::MD_TYPE_QUIT);
					m_MessageDialog.ShowModal();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Display###MainMenu_Display"))
			{
				uint32_t debugDrawFlags = m_Scene->GetDebugDrawFlags();

				bool displayLightEnable = (debugDrawFlags & DEBUG_DRAW_LIGHT_SHAPE) == DEBUG_DRAW_LIGHT_SHAPE;
				if (ImGui::MenuItem("LightShape###MainMenu_Display_LightShape", nullptr, &displayLightEnable) == true)
				{
					if (displayLightEnable == true)
					{
						VE_SET_BIT(debugDrawFlags, DEBUG_DRAW_LIGHT_SHAPE);
					}
					else
					{
						VE_RESET_BIT(debugDrawFlags, DEBUG_DRAW_LIGHT_SHAPE);
					}
				}

				bool displayMeshBoundsEnable = (debugDrawFlags & DEBUG_DRAW_MESH_BOUNDS) == DEBUG_DRAW_MESH_BOUNDS;
				if (ImGui::MenuItem("MeshBounds###MainMenu_Display_MeshBounds", nullptr, &displayMeshBoundsEnable) == true)
				{
					if (displayMeshBoundsEnable == true)
					{
						VE_SET_BIT(debugDrawFlags, DEBUG_DRAW_MESH_BOUNDS);
					}
					else
					{
						VE_RESET_BIT(debugDrawFlags, DEBUG_DRAW_MESH_BOUNDS);
					}
				}

				m_Scene->SetDebugDrawFlags(debugDrawFlags);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Window###MainMenu_Window"))
			{
				ImGui::MenuItem("Fps###MainMenu_Window_Fps", nullptr, m_FpsDialog.GetShowPtr());
				ImGui::MenuItem("Outliner###MainMenu_Window_Outliner", nullptr, m_OutlinerDialog.GetShowPtr());
				ImGui::MenuItem("Inspector###MainMenu_Window_Inspector", nullptr, m_InspectorDialog.GetShowPtr());
				ImGui::MenuItem("Log###MainMenu_Window_Log", nullptr, m_LogDialog.GetShowPtr());
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help###MainMenu_Help"))
			{
				if (ImGui::MenuItem("About###MainMenu_Help_About", nullptr, nullptr) == true)
				{
					m_AboutDialog.ShowModal();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		// File - New...
		if (m_ImportDialog.Render() == ImportDialog::RESULT_OK)
		{
			VE_ASSERT(m_LoadingModel == nullptr);

			CloseProject();

			const ImportDialog::Data& data = m_ImportDialog.GetData();

			m_Project = Project::Create();
			m_Project->New(data.sourceFilePath.c_str());

			StringW projectFilePathW = m_Project->GetFilePathWithoutExtension();
			projectFilePathW += L".xml";

			StringA projectFilePathA;
			ToMultibyteString(projectFilePathW.c_str(), projectFilePathA);

			m_OpenFileBrowser.SetFilePath(projectFilePathA.c_str());

			SkeletalModelPtr model = SkeletalModel::Create(m_DeviceContext);
			m_LoadingModel = model;

			BackgroundJobHandlePtr handle = m_BackgroundQueue->AddImportJob(
				m_BackgroundJobDialog.GetLogger(),
				m_DeviceContext,
				data.sourceFilePath.c_str(), data.sourceConfig, FbxModelSource::Create(),
				data.rednererConfig, model);

			m_BackgroundJobDialog.ShowModel(handle);
		}

		// File - Open...
		if (m_OpenFileBrowser.Render() == FileBrowser::RESULT_OK)
		{
			CloseProject();

			StringW filePath;
			ToWideString(m_OpenFileBrowser.GetFilePath(), filePath);

			m_Project = Project::Create();

			if (m_Project->Open(m_Logger, m_DeviceContext, m_Scene, filePath.c_str()) == true)
			{
				m_OutlinerDialog.SetNode(m_Scene->GetRootNode());
				m_InspectorDialog.SetNode(m_Scene->GetRootNode());
			}
			else
			{
				m_MessageDialog.SetCaption(VE_NAME_A);
				m_MessageDialog.SetType(MessageDialog::TYPE_OK);
				m_MessageDialog.SetText("Failed to load!");
				m_MessageDialog.SetUserIndex(App::MD_TYPE_DO_NOTHING);
				m_MessageDialog.ShowModal();
			}
		}

		// Display - Fps
		m_FpsDialog.Render(m_DeviceContext->GetScreenSize(), static_cast<float>(m_AverageFpsPerSec), static_cast<float>(m_DeltaTime));

		// Window - Outliner
		m_OutlinerDialog.Render();
		if (m_OutlinerDialog.GetResult() == OutlinerDialog::RESULT_SELECT_CHANGED)
		{
			NodePtr node = m_OutlinerDialog.GetNode();
			m_InspectorDialog.SetNode(node);
			m_Scene->Select(node);
		}

		// Window - Inspector
		m_InspectorDialog.Render();

		// Window - Log
		m_LogDialog.Render();

		// Help - About
		m_AboutDialog.Render();

		// Background Job Dialog
		int32_t bjResult = m_BackgroundJobDialog.Render();
		if (bjResult == BACKGROUND_JOB_COMPLETED)
		{
			VE_ASSERT(m_LoadingModel != nullptr);

			NodePtr node = m_Scene->AddNode(m_Scene->GetRootNode(), Project::SceneModelName, Project::SceneModelGroup, m_LoadingModel);
			node->Update();

			m_OutlinerDialog.SetNode(m_LoadingModel->GetRootNode());
			m_InspectorDialog.SetNode(m_LoadingModel->GetRootNode());

			m_LoadingModel = nullptr;
		}

		// Message Dialog
		int32_t mdResult = m_MessageDialog.Render();
		if (mdResult != 0)
		{
			switch (m_MessageDialog.GetUserIndex())
			{
			case App::MD_TYPE_QUIT:
				if (mdResult == MessageDialog::RESULT_YES)
				{
					Shutdown();
				}
				break;
			}
		}

		m_Gui->End();

		/*********/
		/* Scene */
		/*********/

		m_Scene->Update();

		// ----------------------------------------------------------------------------------------------------
		// 描画
		// ----------------------------------------------------------------------------------------------------

		m_DeviceContext->BeginRender();

		/*********/
		/* Scene */
		/*********/

		m_Scene->Render();

		/*******/
		/* GUI */
		/*******/

		m_Gui->Render();

		m_DeviceContext->EndRender();

		// ----------------------------------------------------------------------------------------------------
	}

	void App::OnMouseButtonDown(uint32_t keyFlags, const glm::ivec2 pos)
	{
		ImGuiIO& io = ImGui::GetIO();

		if (io.WantCaptureMouse == false)
		{
			if (keyFlags & MK_RBUTTON)
			{
				NodePtr node = m_Scene->Select(pos);

				m_OutlinerDialog.SetNode(node);
				m_InspectorDialog.SetNode(m_OutlinerDialog.GetNode());
			}
		}
	}

	void App::OnMouseButtonUp(uint32_t keyFlags, const glm::ivec2 pos)
	{
	}

	void App::OnMouseMove(uint32_t keyFlags, const glm::ivec2 pos)
	{
		ImGuiIO& io = ImGui::GetIO();

		glm::vec2 vel = pos - m_MousePos;

		if (io.WantCaptureMouse == false)
		{
			if (keyFlags & MK_MBUTTON)
			{
				CameraPtr camera = m_Scene->GetCamera();

				glm::quat rotation = camera->GetRotation();
				glm::vec3 at = camera->GetAt();
				float distance = camera->GetDistance();

				if (keyFlags & MK_CONTROL)
				{
					distance += static_cast<float>(vel.y) * -m_CameraParam.distanceCoiffe;
					distance = std::max(1.0f, distance);
				}
				else if (keyFlags & MK_SHIFT)
				{
					glm::mat4 mat = glm::toMat4(rotation);

					glm::vec4 dirX(1.0f, 0.0f, 0.0f, 1.0f);
					glm::vec4 dirY(0.0f, 1.0f, 0.0f, 1.0f);

					dirX = mat * dirX;
					dirY = mat * dirY;

					at += glm::vec3(dirX * static_cast<float>(vel.x) * -m_CameraParam.moveCoiffe);
					at += glm::vec3(dirY * static_cast<float>(vel.y) * -m_CameraParam.moveCoiffe);
				}
				else
				{
					glm::quat rot;

					rot = glm::angleAxis(glm::radians(static_cast<float>(vel.x) * -m_CameraParam.rotationCoiffe), glm::vec3(0.0f, 1.0f, 0.0f));
					rot *= glm::angleAxis(glm::radians(static_cast<float>(vel.y) * m_CameraParam.rotationCoiffe), glm::vec3(1.0f, 0.0f, 0.0f));

					rotation = glm::normalize(rotation * rot);
				}

				camera->SetView(rotation, at, distance);
				m_CameraNode->Update();
			}
		}

		m_MousePos = pos;
	}

	void App::OnMouseWheel(uint32_t keyFlags, int16_t delta, const glm::ivec2 pos)
	{
		ImGuiIO& io = ImGui::GetIO();

		if (io.WantCaptureMouse == false)
		{
			if (delta != 0)
			{
				CameraPtr camera = m_Scene->GetCamera();

				const glm::quat& rotation = camera->GetRotation();
				const glm::vec3& at = camera->GetAt();
				float distance = camera->GetDistance();

				if (delta > 0)
				{
					distance -= m_CameraParam.distanceCoiffe;
					distance = std::max(1.0f, distance);
				}
				else if (delta < 0)
				{
					distance += m_CameraParam.distanceCoiffe;
				}

				camera->SetView(rotation, at, distance);
				m_CameraNode->Update();
			}
		}
	}

	void App::CloseProject()
	{
		m_DeviceContext->WaitIdle();

		m_Project = nullptr;

		m_OutlinerDialog.SetNode(nullptr);
		m_InspectorDialog.SetNode(nullptr);

		m_Scene->RemoveNodeByGroup(m_Scene->GetRootNode(), 1);

		m_DeviceContext->WaitIdle();
	}

	void App::Shutdown()
	{
		PostMessage(m_WindowHandle, WM_DESTROY, 0, 0);
	}

	LRESULT CALLBACK App::WindowProc(HWND windowHandle, UINT message, WPARAM wparam, LPARAM lparam)
	{
		glm::ivec2 pos;
		uint32_t keyFlags;
		int16_t delta;

		ve::App::s_pThis->m_Gui->WndProc(windowHandle, message, wparam, lparam);

		switch (message)
		{
		case  WM_SIZE:
			if (ve::App::s_pThis->m_CameraNode != nullptr)
			{
				ve::App::s_pThis->m_CameraNode->Update();
			}
			break;

		case WM_MOUSEMOVE:
			pos.x = GET_X_LPARAM(lparam);
			pos.y = GET_Y_LPARAM(lparam);
			ve::App::s_pThis->OnMouseMove(static_cast<uint32_t>(wparam), pos);
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
			pos.x = GET_X_LPARAM(lparam);
			pos.y = GET_Y_LPARAM(lparam);
			ve::App::s_pThis->OnMouseButtonDown(static_cast<uint32_t>(wparam), pos);
			break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
			pos.x = GET_X_LPARAM(lparam);
			pos.y = GET_Y_LPARAM(lparam);
			ve::App::s_pThis->OnMouseButtonUp(static_cast<uint32_t>(wparam), pos);
			break;

		case WM_MOUSEWHEEL:
			keyFlags = GET_KEYSTATE_WPARAM(wparam);
			delta = GET_WHEEL_DELTA_WPARAM(wparam);
			pos.x = GET_X_LPARAM(lparam);
			pos.y = GET_Y_LPARAM(lparam);
			ve::App::s_pThis->OnMouseWheel(keyFlags, delta, pos);
			break;

		case WM_DESTROY:
			if (ve::App::s_pThis != nullptr)
			{
				ve::App::s_pThis->OnFinalize();
			}
			PostQuitMessage(0);
			break;
		}

		return DefWindowProc(windowHandle, message, wparam, lparam);
	}

	/*********************************************/
	/* private - override IDeviceContextListener */
	/*********************************************/

	void App::OnLost()
	{
		m_Scene->Lost();
	}

	void App::OnRestore()
	{
		if (m_Scene->Restore() == false)
		{
		}
	}

	void App::OnCriticalError()
	{
		MessageBox(m_WindowHandle, L"致命的なエラーが発生しました。アプリケーションを終了します。", VE_NAME_W, MB_ICONERROR | MB_OK);

		m_State = App::STATE_SHUTDOWN;
		Shutdown();
	}

}