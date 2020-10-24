#pragma once
#include "RenderQueuePass.h"
#include "RootSignature.h"
#include "PSO.h"

namespace custom
{
	class CommandContext;
}

class OutlineDrawingPass : public RenderQueuePass
{
	OutlineDrawingPass(std::string pName, UINT BufferWidth, UINT BufferHeight);

	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;

private:
	custom::RootSignature m_RootSignature;
	GraphicsPSO m_GraphicsPSO;
};