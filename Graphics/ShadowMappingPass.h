#pragma once
#include "RenderQueuePass.h"

#include "ShadowCamera.h"

class GraphicsPSO;

namespace custom
{
	class CommandContext;
	class RootSignature;
}

class ShadowMappingPass final : public ID3D12RenderQueuePass
{
public:
	ShadowMappingPass(std::string pName, custom::RootSignature* pRootSignature = nullptr, GraphicsPSO* pShadowPSO = nullptr);
	~ShadowMappingPass();
	void ExecutePass() DEBUG_EXCEPT final;

private:
	static ShadowMappingPass* s_pShadowMappingPass;

private:
	custom::RootSignature* m_pRootSignature;
	GraphicsPSO* m_pShadowPSO;
	ShadowCamera m_ShadowCamera; // For Calculation.

	bool m_bAllocateRootSignature;
	bool m_bAllocatePSO;
};