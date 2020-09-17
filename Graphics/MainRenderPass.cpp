#include "stdafx.h"
#include "MainRenderPass.h"
#include "Device.h"
#include "CommandQueueManager.h"
#include "CommandContext.h"
#include "RootSignature.h"
#include "PSO.h"
#include "PreMadePSO.h"

#include "ColorBuffer.h"
#include "BufferManager.h"

#include "IBaseCamera.h"
#include "Camera.h"

// MainRenderShader File


MainRenderPass::MainRenderPass(const char* Name, custom::RootSignature* pRootSignature/* = nullptr*/, GraphicsPSO* pMainRenderPSO/* = nullptr*/)
	: RenderQueuePass(Name), m_pRootSignature(pRootSignature), m_pMainRenderPSO(pMainRenderPSO)
{
	if (m_pRootSignature == nullptr)
	{
		m_pRootSignature = new custom::RootSignature();

		m_pRootSignature->Reset(5, 2);
		m_pRootSignature->InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);                         // 
		m_pRootSignature->InitStaticSampler(1, premade::g_SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);                          //
		(*m_pRootSignature)[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b0)                   // 
		(*m_pRootSignature)[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b0)                   //               // 
		(*m_pRootSignature)[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);  // 

		(*m_pRootSignature)[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL); // 

		(*m_pRootSignature)[4].InitAsConstants(1, 2, D3D12_SHADER_VISIBILITY_VERTEX);

		m_pRootSignature->Finalize(L"InitAtMainRenderPass", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	}

	if (m_pMainRenderPSO == nullptr)
	{
		m_pMainRenderPSO = new GraphicsPSO();

		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		m_pMainRenderPSO->SetRootSignature(*m_pRootSignature);
		m_pMainRenderPSO->SetRasterizerState(premade::g_RasterizerDefault);
		m_pMainRenderPSO->SetBlendState(premade::g_BlendDisable);
		m_pMainRenderPSO->SetDepthStencilState(premade::g_DepthStateTestEqual);
		m_pMainRenderPSO->SetInputLayout(_countof(InputElements), InputElements);
		m_pMainRenderPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_pMainRenderPSO->SetVertexShader(g_pDepthVS, sizeof(g_pDepthVS));
		m_pMainRenderPSO->SetPixelShader(g_pDepthPS, sizeof(g_pDepthPS));
		m_pMainRenderPSO->SetRenderTargetFormats(0, &bufferManager::g_SceneColorBuffer.GetFormat(), bufferManager::g_ShadowBuffer.GetFormat());
		m_pMainRenderPSO->Finalize();
	}

}

void MainRenderPass::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.Flush();

	// Make the 3D queue wait for the Compute queue to finish SSAO
	ASSERT(device::g_commandQueueManager.GetGraphicsQueue().StallForProducer(device::g_commandQueueManager.GetComputeQueue()));

	// Must Set pfnSetupGraphicsState();

}