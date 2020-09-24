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
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void Reset() DEBUG_EXCEPT override;
	// Binding�ȰŴ� �Һ�(Immutable)�ؾ��ϴ°� ���

private:
	// std::vector<Job> m_Jobs; At RenderQueuePass
	// Job { pDrawable, Step(vector<RenderingResource>) } -> Draw�ѹ��ϱ� ���� ����

	custom::RootSignature* m_RootSignature;
	GraphicsPSO* m_DepthPSO;
};