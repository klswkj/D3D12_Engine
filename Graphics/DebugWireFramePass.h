#pragma once
#include "RenderQueuePass.h"
#include "RootSignature.h"
#include "PSO.h"

namespace custom
{
	class CommandContext;
}

class DebugWireFramePass final : public ID3D12RenderQueuePass
{
public:
	DebugWireFramePass(std::string Name);
	void ExecutePass() DEBUG_EXCEPT final;

private:
	static DebugWireFramePass* s_pDebugWireFramePass;

private:
	custom::RootSignature m_RootSignature;
	GraphicsPSO m_PSO;
	D3D12_PRIMITIVE_TOPOLOGY m_Topology;
};