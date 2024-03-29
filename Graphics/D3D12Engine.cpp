#include "stdafx.h"
#include "D3D12Engine.h"

#include "Graphics.h"
#include "Window.h"
#include "MyKeyboard.h"
#include "MyMouse.h"
#include "Profiling.h"
#include "BufferManager.h"
#include "ObjectFilterFlag.h"
#include "CommandContextManager.h"
#include "CommandQueueManager.h"
#include "WindowInput.h"
#include "CpuTime.h"

#include "LightPrePass.h"
#include "SSAOPass.h"
#include "ShadowMappingPass.h"
// #include "imgui_demo.cpp"

D3D12Engine::D3D12Engine(const std::wstring& CommandLine/* = ""*/)
{
	m_MainViewport.Width    = (float)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height   = (float)bufferManager::g_SceneColorBuffer.GetHeight(); 
	m_MainViewport.TopLeftX = 0.0f;
	m_MainViewport.TopLeftY = 0.0f;
	m_MainViewport.MinDepth = 0.0f;
	m_MainViewport.MaxDepth = 1.0f;

	m_MainScissor.left   = 0l;
	m_MainScissor.top    = 0l;
	m_MainScissor.right  = (LONG)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom = (LONG)bufferManager::g_SceneColorBuffer.GetHeight();

	m_MasterRenderGraph.BindMainLightContainer(m_MainLightManager.GetContainer());
	m_CameraManager.SetCaptureBox(&m_ObjModelManager.GetBoundingBox());

	m_ObjModelManager.LinkTechniques(m_MasterRenderGraph);
}

D3D12Engine::D3D12Engine(const std::string& CommandLine)
{
	m_MainViewport.Width    = (float)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height   = (float)bufferManager::g_SceneColorBuffer.GetHeight();
	m_MainViewport.TopLeftX = 0.0f;
	m_MainViewport.TopLeftY = 0.0f;
	m_MainViewport.MinDepth = 0.0f;
	m_MainViewport.MaxDepth = 1.0f;

	m_MainScissor.left   = 0L;
	m_MainScissor.top    = 0L;
	m_MainScissor.right  = (LONG)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom = (LONG)bufferManager::g_SceneColorBuffer.GetHeight();

	m_MasterRenderGraph.BindMainLightContainer(m_MainLightManager.GetContainer());
	m_CameraManager.SetCaptureBox(&m_ObjModelManager.GetBoundingBox());


	m_ObjModelManager.LinkTechniques(m_MasterRenderGraph);
}

D3D12Engine::~D3D12Engine()
{
	// custom::StructuredBuffer::DestroyAllVertexBuffer();
	// custom::ByteAddressBuffer::DestroyIndexAllBuffer();
}

void D3D12Engine::Render()
{
	device::g_commandContextManager.SetViewportAndScissor(m_MainViewport, m_MainScissor);

	BeginFrame();
	m_MasterRenderGraph.ExecuteRenderGraph();
	EndFrame();
	
// #ifdef _DEBUG
// 	if (!(m_CurrentTick % 100ull))
// 	{
// 		system("cls");
// 		printf("Current Index : %lld.\n\n", m_CurrentTick);
// 		m_MasterRenderGraph.ProfilingRenderGraph();
// 
// 		VSConstants temp = device::g_commandContextManager.GetVSConstants();
// 		auto tempX = DirectX::XMFLOAT4(temp.modelToProjection.GetX());
// 		auto tempY = DirectX::XMFLOAT4(temp.modelToProjection.GetY());
// 		auto tempZ = DirectX::XMFLOAT4(temp.modelToProjection.GetZ());
// 		auto tempW = DirectX::XMFLOAT4(temp.modelToProjection.GetW());
// 
// 		printf("model to Projection : \n");
// 		printf("%.5f %.5f %.5f %.5f \n",   tempX.x, tempX.y, tempX.z, tempX.w);
// 		printf("%.5f %.5f %.5f %.5f \n",   tempY.x, tempY.y, tempY.z, tempY.w);
// 		printf("%.5f %.5f %.5f %.5f \n",   tempZ.x, tempZ.y, tempZ.z, tempZ.w);
// 		printf("%.5f %.5f %.5f %.5f \n\n", tempW.x, tempW.y, tempW.z, tempW.w);
// 
// 		printf("Position : %.5f, %.5f, %.5f \n", temp.viewerPos.x, temp.viewerPos.y, temp.viewerPos.z);
// 	}
// #endif

	m_MasterRenderGraph.ResetRenderGraph();
	++m_CurrentTick;
}

bool D3D12Engine::Update(float DeltaTime)
{
	m_ObjModelManager.Submit(eObjectFilterFlag::kOpaque);

	// Profiling::Update(); ASSERT(false);
	windowInput::Update(DeltaTime);
	m_CameraManager.Update(DeltaTime);
	m_MasterRenderGraph.BindMainCamera(m_CameraManager.GetActiveCamera().get());
	m_MasterRenderGraph.UpdateRenderGraph();

	m_MainViewport.Width  = (float)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height = (float)bufferManager::g_SceneColorBuffer.GetHeight();
	m_MainScissor.right   = (LONG)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom  = (LONG)bufferManager::g_SceneColorBuffer.GetHeight();

#ifdef _DEBUG
	if (!(m_CurrentTick % 100ull))
	{
		printf("DeltaTime : %.5f\n\n", DeltaTime);
	}
#endif
	
	return false;
}

int D3D12Engine::Run()
{
	::ShowWindow(window::g_hWnd, SW_SHOWDEFAULT);
	::UpdateWindow(window::g_hWnd);
	while (::ShowCursor(TRUE) < 0);

	window::g_pCWindow->StartMessagePump
	(
		[&]
		{
			HandleInput();
			float FrameTime = graphics::GetFrameTime();
			Update(FrameTime);
			Render();
			graphics::Present();
		}
	);

	device::g_commandQueueManager.WaitIdleCPUSide();

	return 0;
}

void D3D12Engine::HandleInput()
{
	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_escape))
	{
		if (window::bCursorEnabled())
		{
			window::DisableCursor();
			window::g_pCWindow->m_Mouse.EnableRawMouse();
			m_CameraManager.ToggleMomentum();
		}
		else
		{
			window::EnableCursor();
			window::g_pCWindow->m_Mouse.DisableRawMouse();
			m_CameraManager.ToggleMomentum();
		}
	}
	if(windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_delete))
	{
		PostQuitMessage(0);
	}
	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_up))
	{
		m_MainLightManager.GetContainer()->front().Up();
	}
	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_down))
	{
		m_MainLightManager.GetContainer()->front().Down();
	}
	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_left))
	{
		m_MainLightManager.GetContainer()->front().Rotate1();
	}
	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_right))
	{
		m_MainLightManager.GetContainer()->front().Rotate2();
	}
	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_1))
	{
		// TODO : Current Issue.
		// m_MasterRenderGraph.ToggleDebugPasses(); 
	}
	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_2))
	{
		m_MasterRenderGraph.ToggleDebugShadowMap();
	}
	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_3))
	{
		m_MasterRenderGraph.ToggleSSAOPass();
	}
	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_4))
	{
		m_MasterRenderGraph.ToggleSSAODebugMode();
	}
	/*
	const KeyboardEvent _KeyboardEvent = window::g_pCWindow->m_Keyboard.ReadEvent();

	if (_KeyboardEvent.IsPress())
	{
		unsigned char InputChar = window::g_pCWindow->m_Keyboard.ReadChar();

		switch (InputChar)
		{
		case VK_ESCAPE:
		{
			if (window::bCursorEnabled())
			{
				window::DisableCursor();
				window::g_pCWindow->m_Mouse.EnableRawMouse();
			}
			else
			{
				window::EnableCursor();
				window::g_pCWindow->m_Mouse.DisableRawMouse();
			}
			break;
		}
		case '1':
		{
			// m_MasterRenderGraph.ToggleDebugPasses();
			break;
		}
		case '2':
		{
			// m_MasterRenderGraph.ToggleDebugShadowMap();
			break;
		}
		case '3':
		{
			// m_MasterRenderGraph.ToggleSSAOPass();
			break;
		}
		case '4':
		{
			// m_MasterRenderGraph.ToggleSSAODebugMode();
			break;
		}
		}
	}
	*/
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
		custom::GraphicsContext& graphicsContext = custom::GraphicsContext::Begin(1);

		graphicsContext.PIXBeginEvent(L"Render_Imgui", 0);

		m_CameraManager.RenderWindows();
		m_ObjModelManager.RenderComponentWindows();

		graphicsContext.SetViewportAndScissor(bufferManager::g_SceneColorBuffer, 0);
		graphicsContext.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, 0U);
		graphicsContext.SubmitResourceBarriers(0);
		graphicsContext.SetRenderTarget(bufferManager::g_SceneColorBuffer.GetRTV(), 0);
		// graphicsContext.SetViewportAndScissor(0, 0, bufferManager::g_SceneColorBuffer.GetWidth(), bufferManager::g_SceneColorBuffer.GetHeight());

		graphicsContext.GetCommandList(0)->SetDescriptorHeaps(1, &device::g_ImguiFontHeap);
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), graphicsContext.GetCommandList(0));

		graphicsContext.PIXEndEvent(0);
		graphicsContext.Finish(false);
	}
}
