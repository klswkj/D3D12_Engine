#pragma once
#include "RenderQueuePass.h"

namespace custom
{
	class CommandContext;
	class RootSignature;
}

class ShadowCamera;
class GraphicsPSO;

class Z_PrePass : public RenderQueuePass
{
public:
	Z_PrePass(std::string pName, custom::RootSignature* pRootSignature = nullptr, GraphicsPSO* pDepthPSO = nullptr);
	~Z_PrePass();
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	// Binding�ȰŴ� �Һ�(Immutable)�ؾ��ϴ°� ���

private:
	static Z_PrePass* s_pZ_PrePass;

	custom::RootSignature* m_pRootSignature;
	GraphicsPSO* m_pDepthPSO;

	bool m_bAllocateRootSignature = false;
	bool m_bAllocatePSO = false;
};