#include "stdafx.h"
#include "ShadowPrePass.h"
#include "CommandContext.h"
#include "CopyContext.h"
#include "RootSignature.h"
#include "PSO.h"
#include "Camera.h"
#include "ShadowCamera.h"
#include "ShadowBuffer.h"
#include "BufferManager.h"
#include "PreMadePSO.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthVS.h"
// #include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthPS.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/Relase/Graphics(.lib)/CompiledShaders/DepthVS.h"
// #include "../x64/Relase/Graphics(.lib)/CompiledShaders/DepthPS.h"
#endif

ShadowPrePass* ShadowPrePass::sm_pShadowPrePass = nullptr;

ShadowPrePass::ShadowPrePass
(
	std::string pName, 
	custom::RootSignature* pRootSignature/* = nullptr*/, 
	GraphicsPSO* pShadowPSO/* = nullptr*/
)
	: 
	ID3D12RenderQueuePass(pName, JobFactorization::ByNone),
	m_pRootSignature(pRootSignature), 
	m_pShadowPSO(pShadowPSO),
	m_bAllocateRootSignature(false),
	m_bAllocatePSO(false)
{
	ASSERT(sm_pShadowPrePass == nullptr);
	sm_pShadowPrePass = this;

	// TODO : Move to Material.cpp
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
		(*m_pRootSignature)[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);  // Diffuse, Specular, Emissvie, Normal, LightMap, Reflection
		(*m_pRootSignature)[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL); // texSSAO, texShadow, lightBuffer, lightShadowArrayTex, LightGrid, LightGridMask

		m_pRootSignature->Finalize(L"InitAtShadowPrePass_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
		// m_pShadowPSO->SetPixelShader       (g_pDepthPS, sizeof(g_pDepthPS));
		m_pShadowPSO->SetRenderTargetFormats  (0, nullptr, bufferManager::g_ShadowBuffer.GetFormat());
		m_pShadowPSO->Finalize(L"InitAtShadowPrePass_PSO");

		m_bAllocatePSO = true;
	}
}

ShadowPrePass::~ShadowPrePass()
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

void ShadowPrePass::ExecutePass() DEBUG_EXCEPT
{
#ifdef _DEBUG
	graphics::InitDebugStartTick();
	float DeltaTime1 = graphics::GetDebugFrameTime();
#endif

	using namespace bufferManager;

	size_t CommandCount = GetJobCount();
	CommandCount = min(CommandCount, (size_t)m_NumTaskFibers); // m_NumTaskFibers = 3u
	custom::GraphicsContext& graphicsContext = custom::GraphicsContext::Begin((uint8_t)CommandCount + (uint8_t)m_NumTransitionResourceState);
	graphicsContext.SetResourceTransitionBarrierIndex(0u);

	uint8_t StartJobIndex   = 0u;
	uint8_t MaxCommandIndex = graphicsContext.GetNumCommandLists() - 1u; // 3u

	ASSERT((StartJobIndex <= MaxCommandIndex) && MaxCommandIndex < 127u);

	SetParams(&graphicsContext, StartJobIndex, graphicsContext.GetNumCommandLists()); // custom::ThreadPool::EnqueueVariadic(SetParamsWithVariadic, 4, this, &graphicsContext, 1, MaxCommandIndex); -> // custom::ThreadPool::WaitAllFinished(); // Wait SetParams.
	
	graphicsContext.PIXBeginEvent(L"2_ShadowPrePass", 0u);
	graphicsContext.SetRootSignatureRange(*m_pRootSignature, StartJobIndex, MaxCommandIndex);
	graphicsContext.SetPipelineStateRange(*m_pShadowPSO,     StartJobIndex, MaxCommandIndex);

	size_t LightShadowMatrixSize = g_LightShadowMatrixes.size();

#define USE_COPY_CONTEXT

#if defined(USE_COPY_CONTEXT)
	custom::CopyContext& copyContext = custom::CopyContext::Begin(1u);
	copyContext.SetResourceTransitionBarrierIndex(0u);

	// 아래 케이스 같은 경우 CPU측 성능향상????
	copyContext.TransitionResources(g_LightShadowArray, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	copyContext.PIXBeginEvent(L"2_ShadowPrePass_CumulativeShadow_To_g_LightShadowArray", 0u);

#ifdef _DEBUG
	static uint64_t s_LoopIndex = -1;
	s_LoopIndex = 0ull;
	wchar_t PIXBuffer[8];

	static uint64_t s_WorkerThreadIndex = -1;
	wchar_t PIXWorkerThreadBuffer[32];

#endif

	// 여기 한개의 CommandContext를 더 붙이면 더 빡세게 최적화 가능할듯.
	// 3D Queue <-> CopyQueue 전환 최적화해야함. -> 아마 최선의 경우는 CopyQueue쓰지말고 모두 3DQueue로 하는게?

	for (UINT i = 0U; i < LightShadowMatrixSize; ++i)
	{
#ifdef _DEBUG
		swprintf(PIXBuffer, _countof(PIXBuffer), L"Loop #%zu", ++s_LoopIndex);
		graphicsContext.PIXBeginEvent(PIXBuffer, 0u);

		s_WorkerThreadIndex = 0ull;
#endif
		graphicsContext.TransitionResource(g_CumulativeShadowBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, 0U);
		graphicsContext.SubmitResourceBarriers(0u);
		graphicsContext.SetModelToProjection(g_LightShadowMatrixes[i]);

		graphicsContext.SetOnlyDepthStencilRange(g_CumulativeShadowBuffer.GetDSV(), StartJobIndex, MaxCommandIndex);
		graphicsContext.SetViewportAndScissorRange(g_CumulativeShadowBuffer, StartJobIndex, MaxCommandIndex);
		graphicsContext.SetVSConstantsBufferRange(0u, StartJobIndex, MaxCommandIndex);
		graphicsContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
#if defined(_DEBUG)
		s_WorkerThreadIndex = 0ull;

		for (uint8_t i = 0ul; i < graphicsContext.GetNumCommandLists(); ++i)
		{
			swprintf(PIXWorkerThreadBuffer, _countof(PIXWorkerThreadBuffer), L"ShadowPrePass WorkerThread #%zu", ++s_WorkerThreadIndex);
			graphicsContext.PIXBeginEvent(PIXWorkerThreadBuffer, i);
		}
#endif

		custom::ThreadPool::MultipleEnqueue(ContextWorkerThread, (size_t)(MaxCommandIndex - StartJobIndex), (void**)&m_Params[1], sizeof(RenderQueueThreadParameterWrapper));
		ContextWorkerThread(&m_Params[0]);

		custom::ThreadPool::WaitAllFinished(); // Wait ContextWorkerThread

#ifdef _DEBUG
		graphicsContext.PIXEndEventAll(); // End WorkerThread
#endif

		graphicsContext.TransitionResource(g_CumulativeShadowBuffer, D3D12_RESOURCE_STATE_COMMON, 0U); // D3D12_RESOURCE_STATE_COPY_SOURCE D3D12_RESOURCE_STATE_COMMON
		graphicsContext.SubmitResourceBarriers(MaxCommandIndex);
		graphicsContext.PIXEndEvent(MaxCommandIndex);
		graphicsContext.WaitLastExecuteGPUSide(copyContext);
		graphicsContext.ExecuteCommands(MaxCommandIndex, false, (i == (LightShadowMatrixSize - 1)));
		
		copyContext.CopySubresource(g_LightShadowArray, i, g_CumulativeShadowBuffer, 0U, 0u);
		copyContext.WaitLastExecuteGPUSide(graphicsContext);

		if (i == LightShadowMatrixSize - 1)
		{
			copyContext.SubmitResourceBarriers(0u);
			copyContext.PIXEndEvent(0u); // end 2_ShadowPrePass_CumulativeShadow_To_g_LightShadowArray
			copyContext.Finish(false);
		}
		else
		{
			copyContext.SubmitResourceBarriers(0u);
			copyContext.ExecuteCommands(0u, true, true);
		}
	}
	graphicsContext.SetModelToProjection(graphicsContext.GetpMainCamera()->GetViewProjMatrix());
	graphicsContext.PIXEndEvent(0u);
	graphicsContext.SetResourceTransitionBarrierIndex(0u);
	graphicsContext.PauseRecording();

#else
	graphicsContext.TransitionResources(g_LightShadowArray, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

#ifdef _DEBUG
	static uint64_t s_LoopIndex = -1;
	s_LoopIndex = 0ull;
	wchar_t PIXBuffer[8];

	static uint64_t s_WorkerThreadIndex = -1;
	wchar_t PIXWorkerThreadBuffer[32];

#endif
	for (UINT i = 0U; i < LightShadowMatrixSize; ++i)
	{
#ifdef _DEBUG
		swprintf(PIXBuffer, _countof(PIXBuffer), L"Loop #%zu", ++s_LoopIndex);
		graphicsContext.PIXBeginEvent(PIXBuffer, 0u);

		s_WorkerThreadIndex = 0ull;
#endif

		graphicsContext.TransitionResource(g_CumulativeShadowBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, 0U);
		graphicsContext.SubmitResourceBarriers(0u);
		graphicsContext.SetModelToProjection(g_LightShadowMatrixes[i]);

		graphicsContext.SetOnlyDepthStencilRange(g_CumulativeShadowBuffer.GetDSV(), StartJobIndex, MaxCommandIndex);
		graphicsContext.SetViewportAndScissorRange(g_CumulativeShadowBuffer, StartJobIndex, MaxCommandIndex);
		graphicsContext.SetVSConstantsBufferRange(0u, StartJobIndex, MaxCommandIndex);
		graphicsContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#if defined(_DEBUG)
		s_WorkerThreadIndex = 0ull;

		for (uint8_t i = 0ul; i < graphicsContext.GetNumCommandLists(); ++i)
		{
			swprintf(PIXWorkerThreadBuffer, _countof(PIXWorkerThreadBuffer), L"ShadowPrePass WorkerThread #%zu", ++s_WorkerThreadIndex);
			graphicsContext.PIXBeginEvent(PIXWorkerThreadBuffer, i);
		}
#endif

		custom::ThreadPool::MultipleEnqueue(ContextWorkerThread, (size_t)(MaxCommandIndex - StartJobIndex), (void**)&m_Params[1], sizeof(RenderQueueThreadParameterWrapper));
		ContextWorkerThread(&m_Params[0]);

		custom::ThreadPool::WaitAllFinished(); // Wait ContextWorkerThread

#ifdef _DEBUG
		graphicsContext.PIXEndEventAll(); // End WorkerThread
#endif

		graphicsContext.PIXEndEvent(MaxCommandIndex);
		graphicsContext.PIXBeginEvent(L"Copy LightShadowArray to CumulativeShadowBuffer", MaxCommandIndex);
		graphicsContext.CopySubresource(g_LightShadowArray, i, g_CumulativeShadowBuffer, 0U, MaxCommandIndex);

		graphicsContext.SubmitResourceBarriers(0u);
		graphicsContext.PIXEndEvent(MaxCommandIndex); // end Copy
		if ((i == LightShadowMatrixSize - 1))
		{
			graphicsContext.PIXEndEvent(MaxCommandIndex); // end 2_ShadowPrePass_CumulativeShadow_To_g_LightShadowArray
		}
		graphicsContext.ExecuteCommands(MaxCommandIndex, false, (i == LightShadowMatrixSize - 1));
	}

	graphicsContext.SetModelToProjection(graphicsContext.GetpMainCamera()->GetViewProjMatrix());
	graphicsContext.PIXEndEvent(0u);
	graphicsContext.SetResourceTransitionBarrierIndex(0u);
	graphicsContext.PauseRecording();

#endif
#ifdef _DEBUG
	m_DeltaTime = graphics::GetDebugFrameTime() - DeltaTime1;
#endif
}