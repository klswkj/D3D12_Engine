#pragma once
#include "Pass.h"
#include "RootSignature.h"
#include "PSO.h"

class FillLightGridPass : public Pass
{
	friend class MasterRenderGraph;
public:
	FillLightGridPass(std::string pName);
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void Reset() DEBUG_EXCEPT override;

	void RenderWindow();

private:
	custom::RootSignature m_FillLightRootSignature;
	ComputePSO m_FillLightGridPSO_WORK_GROUP_8;
	ComputePSO m_FillLightGridPSO_WORK_GROUP_16;
	ComputePSO m_FillLightGridPSO_WORK_GROUP_24;
	ComputePSO m_FillLightGridPSO_WORK_GROUP_32;

	const uint32_t m_kMinWorkGroupSize{ 8u };
	uint32_t m_WorkGroupSize{ 16u };
};