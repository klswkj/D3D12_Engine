#include "stdafx.h"
#include "Window.h"
#include "WindowInput.h"
#include "Device.h"
#include "Graphics.h"
#include "ImguiManager.h"
#include "OBJFileManager.h"
#include "GpuTime.h"
#include "D3D12Engine.h"

int wmain()
{    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	imguiManager::Initialize();
	window::Initialize();
	windowInput::Initialize();
	device::Initialize();

	graphics::Initialize();
	OBJFileManager::Initialize("OBJ");

	D3D12Engine d3dEngine;

	d3dEngine.Update(0.0003f);
	d3dEngine.Render();

	imguiManager::Destroy();
	// window::Destroy();
	device::Destroy();
	// graphics::Destroy();
}