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

	uint8_t StartJobIndex   = 0u;
	uint8_t MaxCommandIndex = graphicsContext.GetNumCommandLists() - 1; // 3u

	SetParams(&graphicsContext, StartJobIndex, graphicsContext.GetNumCommandLists());
	// custom::ThreadPool::EnqueueVariadic(SetParamsWithVariadic, 4, this, &graphicsContext, 1, MaxCommandIndex);

	graphicsContext.PIXBeginEvent(L"2_ShadowPrePass", 0);
	graphicsContext.SetRootSignatureRange(*m_pRootSignature, StartJobIndex, MaxCommandIndex);
	graphicsContext.SetPipelineStateRange(*m_pShadowPSO, StartJobIndex, MaxCommandIndex);

	size_t LightShadowMatrixSize = g_LightShadowMatrixes.size();

	// 여기서 밖에 g_LightShadowArray Resouce_Transition 안바꿈
	custom::CopyContext& copyContext = custom::CopyContext::Begin(1);
	copyContext.TransitionResource(g_LightShadowArray, D3D12_RESOURCE_STATE_COPY_DEST, (D3D12_RESOURCE_STATES)-1);
	// copyContext.SubmitResourceBarriers(0u);
	copyContext.PIXBeginEvent(L"2_ShadowPrePass_CumulativeShadow_To_g_LightShadowArray", 0u);

	// custom::ThreadPool::WaitAllFinished(); // Wait SetParams.

#ifdef _DEBUG
	static uint64_t s_LoopIndex = -1;
	s_LoopIndex = 0ull;
	wchar_t PIXBuffer[8];
#endif

	// 여기 한개의 CommandContext를 더 붙이면 더 빡세게 최적화 가능할듯.
	for (UINT i = 0; i < LightShadowMatrixSize; ++i)
	{
#ifdef _DEBUG
		swprintf(PIXBuffer, _countof(PIXBuffer), L"Loop #%zu", ++s_LoopIndex);
		graphicsContext.PIXBeginEvent(PIXBuffer, 0u);
#endif
		graphicsContext.TransitionResource(g_CumulativeShadowBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		graphicsContext.SubmitResourceBarriers(0u);
		graphicsContext.SetModelToProjection(g_LightShadowMatrixes[i]);

		graphicsContext.SetOnlyDepthStencilRange(g_CumulativeShadowBuffer.GetDSV(), StartJobIndex, MaxCommandIndex);
		graphicsContext.SetViewportAndScissorRange(g_CumulativeShadowBuffer, StartJobIndex, MaxCommandIndex);
		graphicsContext.SetVSConstantsBufferRange(0u, StartJobIndex, MaxCommandIndex);
		graphicsContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		custom::ThreadPool::MultipleEnqueue(ContextWorkerThread, (uint16_t)(MaxCommandIndex - StartJobIndex), (void**)&m_Params[1], sizeof(RenderQueueThreadParameterWrapper));
		ContextWorkerThread(&m_Params[0]);

		custom::ThreadPool::WaitAllFinished(); // Wait ContextWorkerThread

		graphicsContext.TransitionResource(g_CumulativeShadowBuffer, D3D12_RESOURCE_STATE_COMMON); // D3D12_RESOURCE_STATE_COPY_SOURCE D3D12_RESOURCE_STATE_COMMON
		graphicsContext.SubmitResourceBarriers(MaxCommandIndex);
		graphicsContext.PIXEndEvent(MaxCommandIndex);
		graphicsContext.WaitLastExecuteGPUSide(copyContext);
		graphicsContext.ExecuteCommands(MaxCommandIndex, false, false);
		
		copyContext.CopySubresource(g_LightShadowArray, i, g_CumulativeShadowBuffer, 0U);
		copyContext.WaitLastExecuteGPUSide(graphicsContext);

		if (i == LightShadowMatrixSize - 1)
		{
			copyContext.PIXEndEvent(0u); // end 2_ShadowPrePass_CumulativeShadow_To_g_LightShadowArray
			copyContext.Finish(false);
		}
		else
		{
			copyContext.ExecuteCommands(0u, true, true);
		}
	}

	// 여기 아래 5줄을 마지막 Execute전에 같이 실행시키고 싶다. -> 다음 Pass로 옮기기.
	graphicsContext.SetModelToProjection(graphicsContext.GetpMainCamera()->GetViewProjMatrix());
	graphicsContext.PIXEndEvent(0u);
	graphicsContext.PauseRecording();

#ifdef _DEBUG
	m_DeltaTime = graphics::GetDebugFrameTime() - DeltaTime1;
#endif
}

/*
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

	uint8_t StartJobIndex = 0u;
	uint8_t MaxCommandIndex = graphicsContext.GetNumCommandLists() - 1; // 3u

	SetParams(&graphicsContext, StartJobIndex, graphicsContext.GetNumCommandLists());
	// custom::ThreadPool::EnqueueVariadic(SetParamsWithVariadic, 4, this, &graphicsContext, 1, MaxCommandIndex);

	graphicsContext.PIXBeginEvent(L"2_ShadowPrePass", 0);
	graphicsContext.SetRootSignatureRange(*m_pRootSignature, StartJobIndex, MaxCommandIndex);
	graphicsContext.SetPipelineStateRange(*m_pShadowPSO, StartJobIndex, MaxCommandIndex);

	size_t LightShadowMatrixSize = g_LightShadowMatrixes.size();

	// 여기서 밖에 g_LightShadowArray Resouce_Transition 안바꿈
	custom::CopyContext& copyContext = custom::CopyContext::Begin(1);
	copyContext.TransitionResource(g_LightShadowArray, D3D12_RESOURCE_STATE_COPY_DEST, (D3D12_RESOURCE_STATES)-1);
	copyContext.SubmitResourceBarriers(0u);
	copyContext.PIXBeginEvent(L"2_ShadowPrePass_CumulativeShadow_To_g_LightShadowArray", 0u);

	uint64_t GraphicsCompletedFenceValue = 0;
	uint64_t CopyCompletedFenceValue = 0;

	// custom::ThreadPool::WaitAllFinished(); // Wait SetParams.

	static uint64_t s_LoopIndex = 0ull;
	s_LoopIndex = 0ull;
	wchar_t PIXBuffer[8];

	// 여기 한개의 CommandContext를 더 붙이면 더 빡세게 최적화 가능할듯.
	for (UINT i = 0; i < LightShadowMatrixSize; ++i)
	{
		swprintf(PIXBuffer, _countof(PIXBuffer), L"Loop #%zu", ++s_LoopIndex);
		graphicsContext.PIXBeginEvent(PIXBuffer, 0u);
		graphicsContext.TransitionResource(g_CumulativeShadowBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		graphicsContext.SubmitResourceBarriers(0);
		graphicsContext.SetModelToProjection(g_LightShadowMatrixes[i]);

		graphicsContext.SetOnlyDepthStencilRange(g_CumulativeShadowBuffer.GetDSV(), StartJobIndex, MaxCommandIndex);
		graphicsContext.SetViewportAndScissorRange(g_CumulativeShadowBuffer, StartJobIndex, MaxCommandIndex);
		graphicsContext.SetVSConstantsBufferRange(0u, StartJobIndex, MaxCommandIndex);
		graphicsContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		custom::ThreadPool::MultipleEnqueue(ContextWorkerThread, (uint16_t)(MaxCommandIndex - StartJobIndex), (void**)&m_Params[1], sizeof(RenderQueueThreadParameterWrapper));
		ContextWorkerThread(&m_Params[0]);

		custom::ThreadPool::WaitAllFinished(); // Wait ContextWorkerThread

		graphicsContext.TransitionResource(g_CumulativeShadowBuffer, D3D12_RESOURCE_STATE_COMMON);
		graphicsContext.SubmitResourceBarriers(MaxCommandIndex);
		graphicsContext.WaitLastExecuteGPUSide(copyContext);
		graphicsContext.PIXEndEvent(MaxCommandIndex);
		graphicsContext.ExecuteCommands(MaxCommandIndex, false);

		copyContext.WaitLastExecuteGPUSide(graphicsContext);
		copyContext.CopySubresource(g_LightShadowArray, i, g_CumulativeShadowBuffer, 0u);

		if (i == LightShadowMatrixSize - 1)
		{
			copyContext.PIXEndEvent(0u); // end 2_ShadowPrePass_CumulativeShadow_To_g_LightShadowArray
			copyContext.Finish(false);
		}
		else
		{
			copyContext.ExecuteCommands(0u, true);
		}
	}

	// 여기 아래 5줄을 마지막 Execute전에 같이 실행시키고 싶다. -> 다음 Pass로 옮기기.
	graphicsContext.SetModelToProjection(graphicsContext.GetpMainCamera()->GetViewProjMatrix());
	graphicsContext.PIXEndEvent(0u);
	graphicsContext.PauseRecording();

#ifdef _DEBUG
	m_DeltaTime = graphics::GetDebugFrameTime() - DeltaTime1;
#endif
}

*/
