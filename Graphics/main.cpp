#include "stdafx.h"
#include "main.h"
#include "Window.h"
#include "Device.h"

namespace engine
{
	void RunApplication(IGameApp& app, const wchar_t* className)
	{
		Init(app);
	}

	void Init(IGameApp& app)
	{
		window::Initialize(); // window, device
		device::Initialize(); // 
		graphics::Initialize(); // PSO, RootSignature, Sampler, commandManager, comandContext, DescriptorAllocator, IMGUI

		app.Init();
	}
}