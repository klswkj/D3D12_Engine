#include "stdafx.h"
#include "main.h"
#include "Window.h"
#include "Device.h"
#include "Graphics.h"

class ID3D12App;

namespace engine
{
	void RunApplication(ID3D12App& app, const wchar_t* className)
	{
		Initialize(app);
	}

	void Initialize(ID3D12App& app)
	{
		if (!bInit)
		{
			window::Initialize(); // window
			device::Initialize(); // 
			graphics::Initialize(); // PSO, RootSignature, Sampler, commandManager, comandContext, DescriptorAllocator, IMGUI

			bInit = true;
		}

		app.StartUp(); // 여기서 App 초기화. 
		// Light, RenderGraph, Camera,
		// Model, Primitive_Figure Init.
		// 
		// And ReadyMadePSO, DXGI_FORMAT, INPUT_ELEMENT_DESC, 
	
	}

	BOOL bInit{ false };
}