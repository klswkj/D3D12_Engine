#pragma once
#include "Pass.h"
#include "RootSignature.h"
#include "PSO.h"

class FillLightGridPass final : public ID3D12ScreenPass
{
	friend class MasterRenderGraph;
public:
	FillLightGridPass(std::string name);
	void ExecutePass() DEBUG_EXCEPT final;
	void Reset() DEBUG_EXCEPT final;

	void RenderWindow();

private:
	static FillLightGridPass* s_pFillLightGridPass;

private:
	custom::RootSignature m_FillLightRootSignature;
	ComputePSO m_FillLightGridPSO_WORK_GROUP_8;
	ComputePSO m_FillLightGridPSO_WORK_GROUP_16;
	ComputePSO m_FillLightGridPSO_WORK_GROUP_24;
	ComputePSO m_FillLightGridPSO_WORK_GROUP_32;

	const uint32_t m_kMinWorkGroupSize;
	uint32_t m_WorkGroupSize;
};