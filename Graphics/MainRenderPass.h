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
class MainRenderPass : public RenderQueuePass
{
public:
	MainRenderPass(std::string Name);
	~MainRenderPass();
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void Reset() DEBUG_EXCEPT override;

private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_SSAOShadowSRVs[6];

	bool m_bAllocateRootSignature = false;
	bool m_bAllocatePSO = false;
};


/*

// RenderColor
class MainRenderPass : public RenderQueuePass
{
public:
	MainRenderPass(std::string Name, custom::RootSignature* pRootSignature = nullptr, GraphicsPSO* pMainRenderPSO = nullptr);
	~MainRenderPass();
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void Reset() DEBUG_EXCEPT override;

private:
	custom::RootSignature* m_pRootSignature;
	GraphicsPSO* m_pMainRenderPSO;

	D3D12_CPU_DESCRIPTOR_HANDLE m_SSAOShadowSRVs[6];

	bool m_bAllocateRootSignature = false;
	bool m_bAllocatePSO = false;
};
*/