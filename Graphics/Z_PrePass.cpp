#include "stdafx.h"
#include "Z_PrePass.h"
#include "CommandContext.h"
#include "RootSignature.h"
#include "PSO.h"
#include "ShadowCamera.h"
#include "ShadowBuffer.h"
#include "BufferManager.h"
#include "PreMadePSO.h"

#include "MasterRenderGraph.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthVS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthPS.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/Relase/Graphics(.lib)/CompiledShaders/DepthVS.h"
#include "../x64/Relase/Graphics(.lib)/CompiledShaders/DepthPS.h"
#endif

Z_PrePass* Z_PrePass::s_pZ_PrePass = nullptr;

Z_PrePass::Z_PrePass(std::string pName, custom::RootSignature* pRootSignature/* = nullptr*/, GraphicsPSO* pDepthPSO/* = nullptr*/)
	: RenderQueuePass(pName), m_pRootSignature(pRootSignature), m_pDepthPSO(pDepthPSO)
{
	ASSERT(s_pZ_PrePass == nullptr);
	s_pZ_PrePass = this;

	if (m_pRootSignature == nullptr)
	{
		m_pRootSignature = new custom::RootSignature();

		m_pRootSignature->Reset(6, 2);
		m_pRootSignature->InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		m_pRootSignature->InitStaticSampler(1, premade::g_SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		(*m_pRootSignature)[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b0)
		(*m_pRootSignature)[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b1)
		(*m_pRootSignature)[2].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b0)
		(*m_pRootSignature)[3].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b1)
		(*m_pRootSignature)[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0,  6, D3D12_SHADER_VISIBILITY_PIXEL); // Diffuse, Specular, Emissvie, Normal, LightMap, Reflection
		(*m_pRootSignature)[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL); // texSSAO, texShadow, lightBuffer, lightShadowArrayTex, LightGrid, LightGridMask
		m_pRootSignature->Finalize(L"InitAtZ_PrePass_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		m_bAllocateRootSignature = true;
	}

	if (m_pDepthPSO == nullptr)
	{
		m_pDepthPSO = new GraphicsPSO();

		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		m_pDepthPSO->SetRootSignature        (*m_pRootSignature);
		m_pDepthPSO->SetRasterizerState      (premade::g_RasterizerDefault);
		m_pDepthPSO->SetBlendState           (premade::g_BlendNoColorWrite);
		m_pDepthPSO->SetDepthStencilState    (premade::g_DepthStateReadWrite);
		m_pDepthPSO->SetInputLayout          (_countof(InputElements), InputElements);
		m_pDepthPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_pDepthPSO->SetRenderTargetFormats  (0, nullptr, bufferManager::g_SceneDepthBuffer.GetFormat());
		m_pDepthPSO->SetVertexShader         (g_pDepthVS, sizeof(g_pDepthVS));
		m_pDepthPSO->Finalize(L"InitAtZ_PrePass_PSO");

		m_bAllocatePSO = true;
	}
}

Z_PrePass::~Z_PrePass()
{
	if (m_bAllocateRootSignature && m_pRootSignature != nullptr)
	{
		delete m_pRootSignature;
		m_pRootSignature = nullptr;
	}

	if (m_bAllocatePSO && m_pDepthPSO != nullptr)
	{
		delete m_pDepthPSO;
		m_pDepthPSO = nullptr;
	}
}

void Z_PrePass::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
#ifdef _DEBUG
	graphics::InitDebugStartTick();
	float DeltaTime1 = graphics::GetDebugFrameTime();
#endif

	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.PIXBeginEvent(L"3_Z_PrePass");

	graphicsContext.SetRootSignature(*m_pRootSignature);
	graphicsContext.SetPipelineState(*m_pDepthPSO);

	graphicsContext.TransitionResource(bufferManager::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	graphicsContext.ClearDepthAndStencil(bufferManager::g_SceneDepthBuffer);
	graphicsContext.SetOnlyDepthStencil(bufferManager::g_SceneDepthBuffer.GetDSV());
	graphicsContext.SetViewportAndScissor(bufferManager::g_SceneColorBuffer);

	{
		graphicsContext.SetMainCamera(*graphicsContext.GetpMainCamera());
		// graphicsContext.SetModelToShadow(MasterRenderGraph::s_pMasterRenderGraph->m_pMainLights->begin()->GetShadowMatrix()); // TODO : Edit the Code.
		graphicsContext.SetVSConstantsBuffer(0u);
		// graphicsContext.SetPSConstantsBuffer(2u); // 1u -> 2u
	}
	RenderQueuePass::Execute(BaseContext);

	graphicsContext.PIXEndEvent();
#ifdef _DEBUG
	float DeltaTime2 = graphics::GetDebugFrameTime();
	m_DeltaTime = DeltaTime2 - DeltaTime1;
#endif
}