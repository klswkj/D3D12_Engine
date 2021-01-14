#include "stdafx.h"
#include "Window.h"
#include "WindowInput.h"
#include "Device.h"
#include "Graphics.h"
#include "ImguiManager.h"
#include "OBJFileManager.h"
#include "GpuTime.h"
#include "D3D12Engine.h"

int CALLBACK main
(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, // LPWSTR lpwCmdLine
	int nCmdShow
)
{    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	CRTDEBUG1;

	// _CrtSetBreakAlloc();

	window::Initialize();
	windowInput::Initialize();
	device::Initialize();
	graphics::Initialize();
	imguiManager::Initialize();
	OBJFileManager::Initialize("OBJ");

	D3D12Engine* pD3DEngine = new D3D12Engine(lpCmdLine);
	pD3DEngine->Run();
	delete pD3DEngine;

	window::Destroy();
	imguiManager::Destroy();
	graphics::ShutDown();
	device::Destroy();

	CRTDEBUG2;

	return 0;
}