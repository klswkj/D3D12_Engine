#include "stdafx.h"
#include "Z_PrePass.h"
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

Z_PrePass::Z_PrePass(std::string pName, custom::RootSignature* pRootSignature/* = nullptr*/, GraphicsPSO* pDepthPSO/* = nullptr*/)
	: RenderQueuePass(pName), m_RootSignature(pRootSignature), m_DepthPSO(pDepthPSO)
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

	if (m_DepthPSO == nullptr)
	{
		m_DepthPSO = new GraphicsPSO();

		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		m_DepthPSO->SetRootSignature(*m_RootSignature);
		m_DepthPSO->SetRasterizerState(premade::g_RasterizerShadow);
		m_DepthPSO->SetBlendState(premade::g_BlendNoColorWrite);
		m_DepthPSO->SetDepthStencilState(premade::g_DepthStateReadWrite);
		m_DepthPSO->SetInputLayout(_countof(InputElements), InputElements);
		m_DepthPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_DepthPSO->SetRenderTargetFormats(0, nullptr, bufferManager::g_SceneDepthBuffer.GetFormat());
		m_DepthPSO->SetVertexShader(g_pDepthVS, sizeof(g_pDepthVS));
		m_DepthPSO->Finalize();
	}
}

void Z_PrePass::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	// No Root Signature has been set, so setting a root cbv doesn't make sense and is invalid.[ EXECUTION ERROR #710: SET_ROOT_CONSTANT_BUFFER_VIEW_INVALID]
	graphicsContext.SetRootSignature(*m_RootSignature);

	graphicsContext.SetVSConstantsBuffer(0u);
	graphicsContext.SetPSConstantsBuffer(1u);

	graphicsContext.TransitionResource(bufferManager::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	graphicsContext.ClearDepth(bufferManager::g_SceneDepthBuffer);
	graphicsContext.SetPipelineState(*m_DepthPSO);
	graphicsContext.SetOnlyDepthStencil(bufferManager::g_SceneDepthBuffer.GetDSV());
	// graphicsContext.SetViewportAndScissor(m_pMainViewport, m_pMainScissor);

	RenderQueuePass::Execute(BaseContext);
}
void Z_PrePass::Reset() DEBUG_EXCEPT
{

}