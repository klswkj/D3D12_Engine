#include "stdafx.h"
#include "RawPSO.h"
#include "CommandContext.h"

RawPSO::RawPSO(ID3D12PipelineState* pPSO)
	: m_pPSO(pPSO)
{
}

void RawPSO::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	BaseContext.SetPipelineStateByPtr(m_pPSO);
}