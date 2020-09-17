#include "stdafx.h"
#include "Window.h"
#include "Device.h"
#include "Graphics.h"
#include "ImguiManager.h"
#include "OBJFileManager.h"
#include "GpuTime.h"

int wmain()
{    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	imguiManager::Initialize();
	window::Initialize();
	device::Initialize();
	// graphics::Initialize();
	GpuTime::Initialize();
	OBJFileManager::Initialize("OBJ");

	imguiManager::Destroy();
	// window::Destroy();
	device::Destroy();
	// graphics::Destroy();
}