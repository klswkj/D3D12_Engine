#pragma once
#include "RenderingResource.h"

namespace custom
{
	class CommandContext;
}

class RawPSO : public RenderingResource
{
	RawPSO(ID3D12PipelineState* pPSO);
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;

private:
	ID3D12PipelineState* m_pPSO;
};