#pragma once

#include "CameraManager.h"
#include "ObjModelManager.h"
#include "MainLightManager.h"
#include "MasterRenderGraph.h"

namespace custom
{
	class CommandContext;
}

class D3D12Engine
{
public:
	D3D12Engine(const std::wstring& CommandLine = L"");
	D3D12Engine(const std::string& CommandLine = "");

	int Run();
	void Render();
	bool Update(float DeltaTime); 

	void HandleInput();
private:
	void BeginFrame();
	void EndFrame();

private:
	CameraManager m_CameraManager;
	ObjModelManager m_ObjModelManager;
	MainLightManager m_MainLightManager;
	MasterRenderGraph m_MasterRenderGraph;

	D3D12_VIEWPORT m_MainViewport;
	D3D12_RECT m_MainScissor;

	BOOL m_bEnableImgui = true;
	bool m_bImguiDemo = true;
#ifdef _DEBUG
	uint64_t m_CurrentIndex = 0;
#endif
};