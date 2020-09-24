#pragma once
#include "stdafx.h"
#include "RenderQueuePass.h"

#include "ShadowCamera.h"
#include "Vector.h"

#include "RenderTarget.h"
#include "DepthStencil.h"

class GraphicsPSO;

namespace custom
{
	class CommandContext;
	class RootSignature;
}

class ShadowMappingPass : public RenderQueuePass
{
public:
	ShadowMappingPass(std::string pName, custom::RootSignature* pRootSignature = nullptr, GraphicsPSO* pShadowPSO = nullptr);

	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void Reset() DEBUG_EXCEPT override;

private:
	custom::RootSignature* m_pRootSignature;
	GraphicsPSO* m_pShadowPSO{ nullptr };
	ShadowCamera m_ShadowCamera;

	// TODO : MasterRenderGraph ���ϳ�, D3D12Engine :: MainLightManager �����̳ʷ� �Ʒ� 5�� ��� ��ü�� ��.

	float m_SunOrientation{ -0.5f };
	float m_SunInclination{ 0.75f };

	Math::Vector3 m_LightDirection; 
	Math::Vector3 m_ShadowCenter;
	Math::Vector3 m_ShadowBounds;
	uint32_t m_BufferPrecision;
};