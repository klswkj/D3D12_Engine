#include "stdafx.h"
#include "ShadowPrePass.h"
#include "CommandContext.h"
#include "RootSignature.h"
#include "PSO.h"
#include "ShadowCamera.h"
#include "ShadowBuffer.h"
#include "BufferManager.h"
#include "PreMadePSO.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthVS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthPS.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/Relase/Graphics(.lib)/CompiledShaders/DepthVS.h"
#include "../x64/Relase/Graphics(.lib)/CompiledShaders/DepthPS.h"
#endif

ShadowPrePass::ShadowPrePass(std::string pName, custom::RootSignature* pRootSignature/* = nullptr*/, GraphicsPSO* pShadowPSO/* = nullptr*/)
	: RenderQueuePass(pName), m_RootSignature(pRootSignature), m_ShadowPSO(pShadowPSO)
{
	if (m_RootSignature == nullptr)
	{
		m_RootSignature = new custom::RootSignature();

		m_RootSignature->Reset(4, 2);
		m_RootSignature->InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);                         // 
		m_RootSignature->InitStaticSampler(1, premade::g_SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);                          //
		(*m_RootSignature)[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b0)                   // 
		(*m_RootSignature)[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b0)                   //               // 
		(*m_RootSignature)[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);  // 
		(*m_RootSignature)[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL); // 
		
		m_RootSignature->Finalize(L"InitAtShadowPrePass", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	}

	if (m_ShadowPSO == nullptr)
	{
		m_ShadowPSO = new GraphicsPSO();

		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		m_ShadowPSO->SetRootSignature(*m_RootSignature);
		m_ShadowPSO->SetRasterizerState(premade::g_RasterizerShadow);
		m_ShadowPSO->SetBlendState(premade::g_BlendNoColorWrite);
		m_ShadowPSO->SetDepthStencilState(premade::g_DepthStateReadWrite);
		m_ShadowPSO->SetInputLayout(_countof(InputElements), InputElements);
		m_ShadowPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_ShadowPSO->SetVertexShader(g_pDepthVS, sizeof(g_pDepthVS));
		m_ShadowPSO->SetPixelShader(g_pDepthPS, sizeof(g_pDepthPS));
		m_ShadowPSO->SetRenderTargetFormats(0, nullptr, bufferManager::g_ShadowBuffer.GetFormat());
		m_ShadowPSO->Finalize();
	}
}

// If LightShadowMatrixSize
void ShadowPrePass::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.TransitionResource(bufferManager::g_LightShadowArray, D3D12_RESOURCE_STATE_COPY_DEST);

	ShadowBuffer& CumulativeLightShadowBuffer = bufferManager::g_CumulativeShadowBuffer;
	size_t LightShadowMatrixSize = bufferManager::g_LightShadowMatrixes.size();

	graphicsContext.SetPipelineState(*m_ShadowPSO);

	for (size_t i = 0; i < LightShadowMatrixSize; ++i)
	{
		CumulativeLightShadowBuffer.BeginRendering(graphicsContext);

		graphicsContext.SetModelToProjection(bufferManager::g_LightShadowMatrixes[i]);
		graphicsContext.SetVSConstantsBuffer(0u);

		RenderQueuePass::Execute(BaseContext);

		CumulativeLightShadowBuffer.EndRendering(graphicsContext); // Transition D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE

		graphicsContext.TransitionResource(CumulativeLightShadowBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);

		graphicsContext.CopySubresource(bufferManager::g_LightShadowArray, i, CumulativeLightShadowBuffer, 0);
	}

	graphicsContext.TransitionResource(bufferManager::g_LightShadowArray, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
void ShadowPrePass::Reset() DEBUG_EXCEPT
{
	
}