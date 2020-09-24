#pragma once

#include "CameraManager.h"
#include "ObjModelManager.h"
#include "MainLightManager.h"
#include "MasterRenderGraph.h"

class D3D12Engine
{
public:
	D3D12Engine(const std::string CommandLine = "");

	void Render(); // MasterRenderGraph에서 그려오는거 받아오기
	void Update(float DeltaTime); // 여기서 Input 받고, 카메라 업데이트하고, 
	               // Imgui 그리기.

private:
	CameraManager m_CameraManager;
	ObjModelManager m_ObjModelManager; // ModelPass에 Submit, LinkTechnique, ModelComponentWindow
	MainLightManager m_MainLightManager;
	MasterRenderGraph m_MasterRenderGraph; // 그리기만

	D3D12_VIEWPORT m_MainViewport;
	D3D12_RECT m_MainScissor;
};

// RenderWindow()랑, RenderComponentWindow() 구분하기

// RenderWindow()는 새로 창을 하나 파는거고, 
// RenderComponentWindow()는 창 옆에 Addon으로 붙어나오는거(Child)

/*
1>LINK : warning LNK4098: defaultlib 'LIBCMTD' conflicts with use of other libs; use /NODEFAULTLIB:library
1>zlibstatic.lib(adler32.obj) : warning LNK4099: PDB 'zlibstatic.pdb' was not found with 'zlibstatic.lib(adler32.obj)' or at 'D:\Visual_Studios_Repository\D3D12_Engine\x64\Debug\zlibstatic.pdb'; linking object as if no debug info
1>zlibstatic.lib(crc32.obj) : warning LNK4099: PDB 'zlibstatic.pdb' was not found with 'zlibstatic.lib(crc32.obj)' or at 'D:\Visual_Studios_Repository\D3D12_Engine\x64\Debug\zlibstatic.pdb'; linking object as if no debug info
1>zlibstatic.lib(inflate.obj) : warning LNK4099: PDB 'zlibstatic.pdb' was not found with 'zlibstatic.lib(inflate.obj)' or at 'D:\Visual_Studios_Repository\D3D12_Engine\x64\Debug\zlibstatic.pdb'; linking object as if no debug info
1>zlibstatic.lib(inftrees.obj) : warning LNK4099: PDB 'zlibstatic.pdb' was not found with 'zlibstatic.lib(inftrees.obj)' or at 'D:\Visual_Studios_Repository\D3D12_Engine\x64\Debug\zlibstatic.pdb'; linking object as if no debug info
1>zlibstatic.lib(inffast.obj) : warning LNK4099: PDB 'zlibstatic.pdb' was not found with 'zlibstatic.lib(inffast.obj)' or at 'D:\Visual_Studios_Repository\D3D12_Engine\x64\Debug\zlibstatic.pdb'; linking object as if no debug info
1>zlibstatic.lib(zutil.obj) : warning LNK4099: PDB 'zlibstatic.pdb' was not found with 'zlibstatic.lib(zutil.obj)' or at 'D:\Visual_Studios_Repository\D3D12_Engine\x64\Debug\zlibstatic.pdb'; linking object as if no debug info
*/