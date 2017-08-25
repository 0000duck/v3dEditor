#include "App.h"

int APIENTRY wWinMain(_In_ HINSTANCE instanceHandle, _In_opt_ HINSTANCE prevInstanceHandle, _In_ LPWSTR pCommandLine, _In_ int commandShow)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif //_DEBUG

	ve::Initialize();

	int32_t ret;
	{
		ve::App app;
		ret = app.Run(instanceHandle);
	}

	ve::Finalize();

	return ret;
}
