#pragma once
#include "RenderQueuePass.h"

class Camera;
class ShadowCamera;
class GraphicsPSO;
namespace custom
{
	class CommandContext;
	class RootSignature;
}

// RenderColor
class MainRenderPass final : public ID3D12RenderQueuePass
{
public:
	MainRenderPass(std::string name, custom::RootSignature* pRootSignature = nullptr, GraphicsPSO* pPSO = nullptr);
	~MainRenderPass();
	void ExecutePass() DEBUG_EXCEPT final;

private:
	static MainRenderPass* s_pMainRenderPass;

	custom::RootSignature* m_pRootSignature;
	GraphicsPSO*           m_pPSO;

	D3D12_CPU_DESCRIPTOR_HANDLE m_SSAOShadowSRVs[6];

	bool m_bAllocateRootSignature;
	bool m_bAllocatePSO;
};