#pragma once
#include "RenderingResource.h"

namespace custom
{
	class CommandContext;
}

class RawPSO final : public RenderingResource
{
	RawPSO(ID3D12PipelineState* pPSO);
	void Bind(custom::CommandContext& BaseContext, uint8_t commandIndex) DEBUG_EXCEPT final;

private:
	ID3D12PipelineState* m_pPSO;
};