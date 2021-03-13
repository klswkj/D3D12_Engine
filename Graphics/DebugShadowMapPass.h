#pragma once
#include "RootSignature.h"
#include "PSO.h"
#include "Pass.h"

namespace custom
{
	class CommandContext;
}

class DebugShadowMapPass final : public D3D12Pass
{
public:
	DebugShadowMapPass(std::string Name);
	~DebugShadowMapPass();
	void ExecutePass() DEBUG_EXCEPT final;

private:
	static DebugShadowMapPass* s_pDebugWireFramePass;

	custom::RootSignature m_RootSignature;
	custom::RootSignature m_CopyRootSignature;
	GraphicsPSO m_PSO;
	ComputePSO m_CopyPSO;
	D3D12_PRIMITIVE_TOPOLOGY m_Topology;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_TopologyType;
	D3D12_VIEWPORT m_ViewPort;
	D3D12_RECT m_ScissorRect;
};