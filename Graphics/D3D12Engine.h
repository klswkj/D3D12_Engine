#pragma once

#include "CameraManager.h"
#include "ObjModelManager.h"
#include "MainLightManager.h"
#include "MasterRenderGraph.h"

class D3D12Engine
{
public:
	D3D12Engine(const std::string CommandLine = "");

	void Render(); // MasterRenderGraph���� �׷����°� �޾ƿ���
	void Update(float DeltaTime); // ���⼭ Input �ް�, ī�޶� ������Ʈ�ϰ�, 
	               // Imgui �׸���.

private:
	CameraManager m_CameraManager;
	ObjModelManager m_ObjModelManager; // ModelPass�� Submit, LinkTechnique, ModelComponentWindow
	MainLightManager m_MainLightManager;
	MasterRenderGraph m_MasterRenderGraph; // �׸��⸸

	D3D12_VIEWPORT m_MainViewport;
	D3D12_RECT m_MainScissor;
};

// RenderWindow()��, RenderComponentWindow() �����ϱ�

// RenderWindow()�� ���� â�� �ϳ� �Ĵ°Ű�, 
// RenderComponentWindow()�� â ���� Addon���� �پ���°�(Child)

/*
1>LINK : warning LNK4098: defaultlib 'LIBCMTD' conflicts with use of other libs; use /NODEFAULTLIB:library
1>zlibstatic.lib(adler32.obj) : warning LNK4099: PDB 'zlibstatic.pdb' was not found with 'zlibstatic.lib(adler32.obj)' or at 'D:\Visual_Studios_Repository\D3D12_Engine\x64\Debug\zlibstatic.pdb'; linking object as if no debug info
1>zlibstatic.lib(crc32.obj) : warning LNK4099: PDB 'zlibstatic.pdb' was not found with 'zlibstatic.lib(crc32.obj)' or at 'D:\Visual_Studios_Repository\D3D12_Engine\x64\Debug\zlibstatic.pdb'; linking object as if no debug info
1>zlibstatic.lib(inflate.obj) : warning LNK4099: PDB 'zlibstatic.pdb' was not found with 'zlibstatic.lib(inflate.obj)' or at 'D:\Visual_Studios_Repository\D3D12_Engine\x64\Debug\zlibstatic.pdb'; linking object as if no debug info
1>zlibstatic.lib(inftrees.obj) : warning LNK4099: PDB 'zlibstatic.pdb' was not found with 'zlibstatic.lib(inftrees.obj)' or at 'D:\Visual_Studios_Repository\D3D12_Engine\x64\Debug\zlibstatic.pdb'; linking object as if no debug info
1>zlibstatic.lib(inffast.obj) : warning LNK4099: PDB 'zlibstatic.pdb' was not found with 'zlibstatic.lib(inffast.obj)' or at 'D:\Visual_Studios_Repository\D3D12_Engine\x64\Debug\zlibstatic.pdb'; linking object as if no debug info
1>zlibstatic.lib(zutil.obj) : warning LNK4099: PDB 'zlibstatic.pdb' was not found with 'zlibstatic.lib(zutil.obj)' or at 'D:\Visual_Studios_Repository\D3D12_Engine\x64\Debug\zlibstatic.pdb'; linking object as if no debug info
*/