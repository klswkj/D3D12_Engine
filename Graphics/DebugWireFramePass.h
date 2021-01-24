#pragma once
#include "RenderQueuePass.h"
#include "RootSignature.h"
#include "PSO.h"

namespace custom
{
	class CommandContext;
}

class DebugWireFramePass : public RenderQueuePass
{
public:
	DebugWireFramePass(std::string Name);
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;

private:
	static DebugWireFramePass* s_pDebugWireFramePass;

	custom::RootSignature m_RootSignature;
	GraphicsPSO m_PSO;
	D3D12_PRIMITIVE_TOPOLOGY m_Topology;
};