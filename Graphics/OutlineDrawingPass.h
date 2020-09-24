#pragma once
#include "RenderQueuePass.h"

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