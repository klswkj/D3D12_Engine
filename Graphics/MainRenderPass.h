#pragma once
#include "RenderQueuePass.h"

class Camera;
class ShadowCamera;

namespace custom
{
	class CommandContext;
}

// RenderColor
class MainRenderPass : public RenderQueuePass
{
public:
	MainRenderPass(const char* Name, custom::RootSignature* pRootSignature = nullptr, GraphicsPSO* pMainRenderPSO = nullptr);

	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void Reset() DEBUG_EXCEPT override;

private:
	custom::RootSignature* m_pRootSignature;
	GraphicsPSO* m_pMainRenderPSO{ nullptr };
};