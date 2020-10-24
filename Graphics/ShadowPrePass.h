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
	void Reset() DEBUG_EXCEPT override;
	// Binding된거는 불변(Immutable)해야하는거 명심

private:
	// std::vector<Job> m_Jobs; At RenderQueuePass
	// Job { pDrawable, Step(vector<RenderingResource>) } -> Draw한번하기 위한 단위

	custom::RootSignature* m_RootSignature;
	GraphicsPSO* m_ShadowPSO;

	bool m_bAllocateRootSignature = false;
	bool m_bAllocatePSO = false;
};

