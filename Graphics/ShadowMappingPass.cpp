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
#include "Camera.h"
#include "MainLight.h"
#include "MasterRenderGraph.h"
#include "SSAOPass.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthVS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthPS.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/Relase/Graphics(.lib)/CompiledShaders/DepthVS.h"
#include "../x64/Relase/Graphics(.lib)/CompiledShaders/DepthPS.h"
#endif

ShadowMappingPass* ShadowMappingPass::s_pShadowMappingPass = nullptr;

ShadowMappingPass::ShadowMappingPass(std::string pName, custom::RootSignature* pRootSignature/* = nullptr*/, GraphicsPSO* pShadowPSO/* = nullptr*/)
	: 
	ID3D12RenderQueuePass(pName, JobFactorization::ByNone), 
	m_pRootSignature(pRootSignature), 
	m_pShadowPSO(pShadowPSO),
	m_bAllocateRootSignature(false),
	m_bAllocatePSO(false)
{
	ASSERT(s_pShadowMappingPass == nullptr);

	s_pShadowMappingPass = this;

	if (m_pRootSignature == nullptr)
	{
		m_pRootSignature = new custom::RootSignature();

		m_pRootSignature->Reset(6, 2); // (4, 2) -> (5, 2) -> (6, 2)
		m_pRootSignature->InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		m_pRootSignature->InitStaticSampler(1, premade::g_SamplerShadowDesc,  D3D12_SHADER_VISIBILITY_PIXEL);
		(*m_pRootSignature)[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b0)
		(*m_pRootSignature)[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b1)
		(*m_pRootSignature)[2].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b0)
		(*m_pRootSignature)[3].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b1)
		(*m_pRootSignature)[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0,  6, D3D12_SHADER_VISIBILITY_PIXEL); // Diffuse, Specular, Emissvie, Normal, LightMap, Reflection
		(*m_pRootSignature)[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL); // texSSAO, texShadow, lightBuffer, lightShadowArrayTex, LightGrid, LightGridMask

		m_pRootSignature->Finalize(L"InitAtShadowMappingPass_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		m_bAllocateRootSignature = true;
	}

	if (m_pShadowPSO == nullptr)
	{
		m_pShadowPSO = new GraphicsPSO();

		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		m_pShadowPSO->SetRootSignature        (*m_pRootSignature);
		m_pShadowPSO->SetRasterizerState      (premade::g_RasterizerShadow);
		m_pShadowPSO->SetBlendState           (premade::g_BlendNoColorWrite);
		m_pShadowPSO->SetDepthStencilState    (premade::g_DepthStateReadWrite);
		m_pShadowPSO->SetInputLayout          (_countof(InputElements), InputElements);
		m_pShadowPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_pShadowPSO->SetVertexShader         (g_pDepthVS, sizeof(g_pDepthVS));
		// m_pShadowPSO->SetPixelShader       (g_pDepthPS, sizeof(g_pDepthPS)); // If need cutout < 0.5f.
		m_pShadowPSO->SetRenderTargetFormats  (0, nullptr, bufferManager::g_ShadowBuffer.GetFormat());
		m_pShadowPSO->Finalize(L"InitAtShadowMappingPass_PSO");

		m_bAllocatePSO = true;
	}
}

ShadowMappingPass::~ShadowMappingPass()
{
	if (m_bAllocateRootSignature && m_pRootSignature != nullptr)
	{
		delete m_pRootSignature;
		m_pRootSignature = nullptr;
	}

	if (m_bAllocatePSO && m_pShadowPSO != nullptr)
	{
		delete m_pShadowPSO;
		m_pShadowPSO = nullptr;
	}
}

// Pass사이에서 멀티스레딩을 한다면,
// SSAO와 여기와 같이 시작
// 3D(Graphics) -> ShadowMapping
// Main                                           -> GPUWait -> MainRender
// Compute      -> SSAO          -> FillLightGrid
void ShadowMappingPass::ExecutePass() DEBUG_EXCEPT
{
	if (SSAOPass::s_pSSAOPass->m_bDebugDraw)
	{
		return;
	}

#ifdef _DEBUG
	graphics::InitDebugStartTick();
	float DeltaTime1 = graphics::GetDebugFrameTime();
#endif
	custom::GraphicsContext& graphicsContext = custom::GraphicsContext::GetRecentContext();

	uint8_t StartJobIndex = 0u;
	uint8_t MaxCommandIndex = graphicsContext.GetNumCommandLists() - 1;

	SetParams(&graphicsContext, StartJobIndex, graphicsContext.GetNumCommandLists());
	// custom::ThreadPool::EnqueueVariadic(SetParamsWithVariadic, 4u, this, &graphicsContext, StartJobIndex, MaxCommandIndex);

	graphicsContext.PIXBeginEvent(L"6_ShadowMappingPass", StartJobIndex);
	graphicsContext.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
	graphicsContext.TransitionResource(bufferManager::g_ShadowBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	graphicsContext.SubmitResourceBarriers(StartJobIndex);
	graphicsContext.ClearColor(bufferManager::g_SceneColorBuffer, StartJobIndex);
	graphicsContext.ClearDepth(bufferManager::g_ShadowBuffer, StartJobIndex);

	graphicsContext.SetRootSignatureRange(*m_pRootSignature, StartJobIndex, MaxCommandIndex);
	graphicsContext.SetPipelineStateRange(*m_pShadowPSO, StartJobIndex, MaxCommandIndex);

	Camera* pCamera = graphicsContext.GetpMainCamera();
	ASSERT(MasterRenderGraph::s_pMasterRenderGraph->m_pMainLights->size() != 0);
	MainLight* temp = &MasterRenderGraph::s_pMasterRenderGraph->m_pMainLights->front();

	graphicsContext.SetModelToProjection(temp->GetShadowMatrix());
	graphicsContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	graphicsContext.SetVSConstantsBufferRange(0u, StartJobIndex, MaxCommandIndex);
	graphicsContext.SetPSConstantsBufferRange(2u, StartJobIndex, MaxCommandIndex); // 1u -> 2u
	graphicsContext.SetOnlyDepthStencilRange(bufferManager::g_ShadowBuffer.GetDSV(), StartJobIndex, MaxCommandIndex);
	graphicsContext.SetViewportAndScissorRange(bufferManager::g_ShadowBuffer, StartJobIndex, MaxCommandIndex);

	custom::ThreadPool::MultipleEnqueue(ContextWorkerThread, (uint8_t)(MaxCommandIndex - StartJobIndex), (void**)&m_Params[1], sizeof(RenderQueueThreadParameterWrapper));
	ContextWorkerThread(&m_Params[0]);

	graphicsContext.SetMainCamera(*pCamera);
	custom::ThreadPool::WaitAllFinished(); // 여기서부터 기다리는 시간이 너무 아깝다.

	graphicsContext.PIXEndEvent(MaxCommandIndex); // End 6_ShadpwMappingPass
	graphicsContext.ExecuteCommands(MaxCommandIndex, false, true);

	graphicsContext.PauseRecording();
#ifdef _DEBUG
	float DeltaTime2 = graphics::GetDebugFrameTime();
	m_DeltaTime = DeltaTime2 - DeltaTime1;
#endif
}