#pragma once
#include "stdafx.h"
#include "RenderQueuePass.h"

#include "ShadowCamera.h"
#include "Vector.h"

#include "RenderTarget.h"
#include "DepthStencil.h"

class GraphicsPSO;

namespace custom
{
	class CommandContext;
	class RootSignature;
}

class ShadowMappingPass : public RenderQueuePass
{
public:
	ShadowMappingPass(std::string pName, custom::RootSignature* pRootSignature = nullptr, GraphicsPSO* pShadowPSO = nullptr);
	~ShadowMappingPass();
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;

private:
	static ShadowMappingPass* s_pShadowMappingPass;

	custom::RootSignature* m_pRootSignature;
	GraphicsPSO* m_pShadowPSO;
	ShadowCamera m_ShadowCamera; // For Calculation.

	bool m_bAllocateRootSignature = false;
	bool m_bAllocatePSO = false;
};