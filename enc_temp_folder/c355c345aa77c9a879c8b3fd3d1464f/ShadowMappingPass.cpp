#include "stdafx.h"
#include "ShadowMappingPass.h"

#include "SamplerDescriptor.h"
#include "ColorBuffer.h"
#include "ShadowBuffer.h"
#include "BufferManager.h"

#include "CommandContext.h"
#include "RootSignature.h"
#include "PSO.h"
#include "PreMadePSO.h"

#include "MathBasic.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthVS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthPS.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/Relase/Graphics(.lib)/CompiledShaders/DepthVS.h"
#include "../x64/Relase/Graphics(.lib)/CompiledShaders/DepthPS.h"
#endif

ShadowMappingPass::ShadowMappingPass(std::string pName, custom::RootSignature* pRootSignature/* = nullptr*/, GraphicsPSO* pShadowPSO/* = nullptr*/)
	: RenderQueuePass(pName), m_pRootSignature(pRootSignature), m_pShadowPSO(pShadowPSO)
{
	if (m_pRootSignature == nullptr)
	{
		m_pRootSignature = new custom::RootSignature();

		m_pRootSignature->Reset(4, 2);
		m_pRootSignature->InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);                         // 
		m_pRootSignature->InitStaticSampler(1, premade::g_SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);                          //
		(*m_pRootSignature)[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b0)                   // 
		(*m_pRootSignature)[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b0)                   //               // 
		(*m_pRootSignature)[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);  // 
		(*m_pRootSignature)[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL); // 

		m_pRootSignature->Finalize(L"InitAtShadowMappingPass", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	}

	if (m_pShadowPSO == nullptr)
	{
		m_pShadowPSO = new GraphicsPSO();

		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		m_pShadowPSO->SetRootSignature(*m_pRootSignature);
		m_pShadowPSO->SetRasterizerState(premade::g_RasterizerShadow);
		m_pShadowPSO->SetBlendState(premade::g_BlendNoColorWrite);
		m_pShadowPSO->SetDepthStencilState(premade::g_DepthStateReadWrite);
		m_pShadowPSO->SetInputLayout(_countof(InputElements), InputElements);
		m_pShadowPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_pShadowPSO->SetVertexShader(g_pDepthVS, sizeof(g_pDepthVS));
		m_pShadowPSO->SetPixelShader(g_pDepthPS, sizeof(g_pDepthPS));
		m_pShadowPSO->SetRenderTargetFormats(0, nullptr, bufferManager::g_ShadowBuffer.GetFormat());
		m_pShadowPSO->Finalize();
	}

	float costheta = cosf(m_SunOrientation);
	float sintheta = sinf(m_SunOrientation);
	float cosphi = cosf(m_SunInclination * 3.14159f * 0.5f);
	float sinphi = sinf(m_SunInclination * 3.14159f * 0.5f);

	m_LightDirection = Math::Normalize(Math::Vector3(costheta * cosphi, sinphi, sintheta * cosphi));
	m_ShadowCenter = Math::Vector3(0.0f, -500.0f, 0.0f);
	m_ShadowBounds = Math::Vector3(5000.0f, 5000.0f, 3000.0f);
	m_BufferPrecision = 16;
}

/*
D3D12 ERROR: ID3D12CommandList::ResourceBarrier: 
Before state (0x0: D3D12_RESOURCE_STATE_[COMMON|PRESENT]) of resource (0x000001A6A46DF7D0:'Main Color Buffer') 
(subresource: 0) specified by transition barrier does not match with the current resource state 
(0x4: D3D12_RESOURCE_STATE_RENDER_TARGET) (assumed at first use) 
[ RESOURCE_MANIPULATION ERROR #527: RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH]
*/
void ShadowMappingPass::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	// graphicsContext.SetPSConstantsBuffer(1); Be Omitted.
	{
		ColorBuffer& SceneColorBuffer = bufferManager::g_SceneColorBuffer;

		graphicsContext.TransitionResource(SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
		graphicsContext.ClearColor(SceneColorBuffer);
	}

	graphicsContext.SetRootSignature(*m_pRootSignature);
	graphicsContext.SetPipelineState(*m_pShadowPSO);
	// And with Set VB, IB, PrimitiveTopology

	ShadowBuffer& MainShadowBuffer = bufferManager::g_ShadowBuffer;

	// 여기서부터 MainLightContainer Loop시작
	MainShadowBuffer.BeginRendering(graphicsContext);
	{
		m_ShadowCamera.UpdateShadowMatrix
		(
			-m_LightDirection, m_ShadowCenter, m_ShadowBounds,
			MainShadowBuffer.GetWidth(), MainShadowBuffer.GetHeight(), 16
		);

		graphicsContext.SetModelToShadowByShadowCamera(m_ShadowCamera);

		RenderQueuePass::Execute(BaseContext);
	}
	MainShadowBuffer.EndRendering(graphicsContext);
}
// PointLightManager(SunLight) 만들고 하자. => 일단 만들고 MainLightManager 만들자.

void ShadowMappingPass::Reset() DEBUG_EXCEPT
{

}