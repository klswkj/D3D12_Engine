#include "stdafx.h"
#include "D3D12Engine.h"

#include "Graphics.h"
#include "Window.h"
#include "MyKeyboard.h"
#include "MyMouse.h"
#include "Profiling.h"
#include "CommandContext.h"
#include "BufferManager.h"
#include "ObjectFilterFlag.h"
#include "CommandContextManager.h"
#include "WindowInput.h"
#include "CpuTime.h"

#include "LightPrePass.h"
#include "ShadowMappingPass.h"
// #include "imgui_demo.cpp"

D3D12Engine::D3D12Engine(const std::wstring& CommandLine/* = ""*/)
{
	m_MainViewport.Width = (float)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height = (float)bufferManager::g_SceneColorBuffer.GetHeight();
	m_MainViewport.MinDepth = 0.0f;
	m_MainViewport.MaxDepth = 1.0f;

	m_MainScissor.left = 0;
	m_MainScissor.top = 0;
	m_MainScissor.right = (LONG)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom = (LONG)bufferManager::g_SceneColorBuffer.GetHeight();

	m_MasterRenderGraph.BindMainLightContainer(m_MainLightManager.GetContainer());

	m_ObjModelManager.LinkTechniques(m_MasterRenderGraph);

	ImGui_ImplDX12_Init
	(
		device::g_pDevice, device::g_DisplayBufferCount, DXGI_FORMAT_R8G8B8A8_UNORM,
		device::g_ImguiFontHeap->GetCPUDescriptorHandleForHeapStart(),
		device::g_ImguiFontHeap->GetGPUDescriptorHandleForHeapStart()
		);
}

D3D12Engine::D3D12Engine(const std::string& CommandLine)
{
	m_MainViewport.Width = (float)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height = (float)bufferManager::g_SceneColorBuffer.GetHeight();
	m_MainViewport.MinDepth = 0.0f;
	m_MainViewport.MaxDepth = 1.0f;

	m_MainScissor.left = 0;
	m_MainScissor.top = 0;
	m_MainScissor.right = (LONG)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom = (LONG)bufferManager::g_SceneColorBuffer.GetHeight();

	m_MasterRenderGraph.BindMainLightContainer(m_MainLightManager.GetContainer());

	m_ObjModelManager.LinkTechniques(m_MasterRenderGraph);

	ImGui_ImplDX12_Init
	(
		device::g_pDevice, device::g_DisplayBufferCount, DXGI_FORMAT_R8G8B8A8_UNORM,
		device::g_ImguiFontHeap->GetCPUDescriptorHandleForHeapStart(),
		device::g_ImguiFontHeap->GetGPUDescriptorHandleForHeapStart()
	);
}

void D3D12Engine::Render()
{
#ifdef _DEBUG
	if (!(m_CurrentIndex % 10))
	{
		// system("cls");
		printf("Current Index : %d.\n\n", m_CurrentIndex);
	}
#endif
	custom::CommandContext& BaseContext = custom::CommandContext::Begin(L"Scene Renderer");

	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.SetViewportAndScissor(m_MainViewport, m_MainScissor);

	m_MasterRenderGraph.Execute(BaseContext);

	m_MasterRenderGraph.Profiling();

	graphicsContext.TransitionResource(bufferManager::g_OverlayBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	graphicsContext.ClearColor(bufferManager::g_OverlayBuffer);
	graphicsContext.SetRenderTarget(bufferManager::g_OverlayBuffer.GetRTV());
	graphicsContext.SetViewportAndScissor(0, 0, bufferManager::g_OverlayBuffer.GetWidth(), bufferManager::g_OverlayBuffer.GetHeight());
	BaseContext.Finish();

	graphics::Present();

	EndFrame();

	ASSERT_HR(device::g_pDXGISwapChain->Present(1, 0));
	
	++m_CurrentIndex;
}

bool D3D12Engine::Update(float DeltaTime)
{
	BeginFrame();
	Profiling::Update();
	windowInput::Update(DeltaTime);
	m_CameraManager.Update(DeltaTime);
	m_MasterRenderGraph.BindMainCamera(m_CameraManager.GetActiveCamera().get());
	m_MasterRenderGraph.Update();

	m_MainViewport.Width  = (float)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height = (float)bufferManager::g_SceneColorBuffer.GetHeight();
	m_MainScissor.right   = (LONG)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom  = (LONG)bufferManager::g_SceneColorBuffer.GetHeight();

	m_ObjModelManager.Submit(eObjectFilterFlag::kOpaque);
	// m_CameraManager.RenderWindows(); // TODO : Add code.

#ifdef _DEBUG
	printf("DeltaTime : %.5f\n\n", DeltaTime);
#endif
	return false;
}

int D3D12Engine::Run()
{
	ShowWindow(window::g_hWnd, SW_SHOWDEFAULT);

	window::StartMessagePump
	(
		[&]
		{
			HandleInput();
			float FrameTime = graphics::GetFrameTime();
			Update(FrameTime);
			Render();
		}
		);

	return 0;
}

void D3D12Engine::HandleInput()
{
	if(windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_delete))
	{
		PostQuitMessage(0);
	}

	const KeyboardEvent _KeyboardEvent = window::g_Keyboard.ReadEvent();
		
	if (_KeyboardEvent.IsPress())
	{
		unsigned char InputChar = window::g_Keyboard.ReadChar();

		switch (InputChar)
		{
		case VK_ESCAPE:
			if (window::bCursorEnabled())
			{
				window::DisableCursor();
				window::g_Mouse.EnableRawMouse();
			}
			else
			{
				window::EnableCursor();
				window::g_Mouse.DisableRawMouse();
			}
			break;
		}
	}
}

void D3D12Engine::BeginFrame()
{
	if (m_bEnableImgui)
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}
}

void D3D12Engine::EndFrame()
{
	if (m_bEnableImgui)
	{
		ImGui::ShowDemoWindow(&m_bImguiDemo);
		ImGui::Render();

		custom::GraphicsContext& ImGuiContext = custom::GraphicsContext::Begin(L"Imgui Renderer");
		ImGuiContext.PIXBeginEvent(L"Render_Imgui");
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), ImGuiContext.GetCommandList());
		ImGuiContext.PIXEndEvent();
		ImGuiContext.Finish();
	}
}