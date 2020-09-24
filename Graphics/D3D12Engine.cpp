#include "stdafx.h"
#include "D3D12Engine.h"

#include "BufferManager.h"
#include "CommandContext.h"
#include "ObjectFilterFlag.h"
#include "CommandContextManager.h"
#include "WindowInput.h"

D3D12Engine::D3D12Engine(const std::string CommandLine/* = ""*/)
{
	m_MainViewport.Width = (float)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height = (float)bufferManager::g_SceneColorBuffer.GetHeight();
	m_MainViewport.MinDepth = 0.0f;
	m_MainViewport.MaxDepth = 1.0f;

	m_MainScissor.left = 0;
	m_MainScissor.top = 0;
	m_MainScissor.right = (LONG)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom = (LONG)bufferManager::g_SceneColorBuffer.GetHeight();

	m_ObjModelManager.LinkTechniques(m_MasterRenderGraph);
}

void D3D12Engine::Render()
{
	custom::CommandContext& BaseContext = custom::CommandContext::Begin(L"Scene Renderer");

	reinterpret_cast<custom::GraphicsContext&>(BaseContext).SetViewportAndScissor(m_MainViewport, m_MainScissor);

	m_MasterRenderGraph.Execute(BaseContext);

	// Window ¶ç¿ì±â

	BaseContext.Finish();
}

void D3D12Engine::Update(float DeltaTime)
{
	m_CameraManager.Update(DeltaTime);

	custom::CommandContext& BaseContext = custom::CommandContext::Begin(L"Update");

	BaseContext.SetModelToProjectionByCamera(*m_CameraManager.GetActiveCamera());
	BaseContext.SetPSConstants(*m_MainLightManager.GetContainer()->begin());
	BaseContext.SetSync();

	m_MainViewport.Width = (float)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height = (float)bufferManager::g_SceneColorBuffer.GetHeight();
	m_MainViewport.MinDepth = 0.0f;
	m_MainViewport.MaxDepth = 1.0f;

	m_MainScissor.left = 0;
	m_MainScissor.top = 0;
	m_MainScissor.right = (LONG)bufferManager::g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom = (LONG)bufferManager::g_SceneColorBuffer.GetHeight();

	m_ObjModelManager.Submit(eObjectFilterFlag::kOpaque);
	// m_CameraManager.RenderWindows(); // Ãß°¡
	
	m_MasterRenderGraph.BindMainCamera(m_CameraManager.GetActiveCamera().get());
}