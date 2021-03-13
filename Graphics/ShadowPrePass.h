#pragma once
#include "Pass.h"
#include "RenderQueuePass.h"

namespace custom
{
	class CommandContext;
	class RootSignature;
}

class ShadowCamera;
class GraphicsPSO;

class ShadowPrePass final : public ID3D12RenderQueuePass
{
public:
	ShadowPrePass(std::string pName, custom::RootSignature* pRootSignature = nullptr, GraphicsPSO* pShadowPSO = nullptr);
	~ShadowPrePass();
	void ExecutePass() DEBUG_EXCEPT final;
	// void RenderWindow() DEBUG_EXCEPT final; Not Used.

private:
	static ShadowPrePass* sm_pShadowPrePass;

private:
	custom::RootSignature* m_pRootSignature;
	GraphicsPSO*           m_pShadowPSO;

	bool m_bAllocateRootSignature;
	bool m_bAllocatePSO;
};

