#pragma once

#include "CameraManager.h"
#include "ObjModelManager.h"
#include "MasterRenderGraph.h"

class D3D12Engine
{
public:
	D3D12Engine(const std::string CommandLine = "");

private:
	CameraManager m_CameraManager;
	ObjModelManager m_ObjModelManager;
	MasterRenderGraph m_MasterRenderGraph;
};