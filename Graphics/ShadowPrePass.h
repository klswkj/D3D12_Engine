#pragma once
#include "RenderQueuePass.h"

namespace custom
{
	class CommandContext;
	class RootSignature;
}

class ShadowCamera;
class GraphicsPSO;

class ShadowPrePass : public RenderQueuePass
{
public:
	ShadowPrePass(std::string pName, custom::RootSignature* pRootSignature = nullptr, GraphicsPSO* pShadowPSO = nullptr);
	~ShadowPrePass();
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;

private:
	static ShadowPrePass* s_pShadowPrePass;

	custom::RootSignature* m_pRootSignature;
	GraphicsPSO*           m_pShadowPSO;

	bool m_bAllocateRootSignature = false;
	bool m_bAllocatePSO = false;
};

