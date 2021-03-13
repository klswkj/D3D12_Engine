#pragma once
#include "RenderQueuePass.h"

namespace custom
{
	class CommandContext;
	class RootSignature;
}

class ShadowCamera;
class GraphicsPSO;

class Z_PrePass : public ID3D12RenderQueuePass
{
public:
	Z_PrePass(std::string pName, custom::RootSignature* pRootSignature = nullptr, GraphicsPSO* pDepthPSO = nullptr);
	~Z_PrePass();
	void ExecutePass() DEBUG_EXCEPT final;

private:
	static Z_PrePass* s_pZ_PrePass;

private:
	custom::RootSignature* m_pRootSignature;
	GraphicsPSO* m_pDepthPSO;

	bool m_bAllocateRootSignature;
	bool m_bAllocatePSO;
};