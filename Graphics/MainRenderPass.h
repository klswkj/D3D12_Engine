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
	MainRenderPass(std::string Name, custom::RootSignature* pRootSignature = nullptr, GraphicsPSO* pPSO = nullptr);
	~MainRenderPass();
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;

private:
	void ExecuteWithSingleThread(custom::CommandContext& BaseContext);
	void ExecuteWithMultiThread();
	void WorkThread(size_t ThreadIndex);

private:
	static MainRenderPass* s_pMainRenderPass;

	custom::RootSignature* m_pRootSignature;
	GraphicsPSO*           m_pPSO;

	D3D12_CPU_DESCRIPTOR_HANDLE m_SSAOShadowSRVs[6];

	bool m_bAllocateRootSignature = false;
	bool m_bAllocatePSO           = false;
	bool m_MultiThreading         = false;

};