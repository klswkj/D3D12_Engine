#include "stdafx.h"
#include "MainRenderPass.h"
#include "Device.h"
#include "CommandQueueManager.h"
#include "CommandContextManager.h"

#include "PreMadePSO.h"
#include "ColorBuffer.h"
#include "BufferManager.h"

#include "IBaseCamera.h"
#include "Camera.h"

#include "MasterRenderGraph.h"
#include "SSAOPass.h"

// MainRenderShader File
#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelVS_Basic.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelVS_TexCoord.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelVS_TexNormal.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelPS_Basic.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelPS_TexCoord.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelPS_TexNormal.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_DiffuseSpecularNormal_PS.h"

#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelViewerPS.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelVS_Basic.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelVS_TexCoord.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelVS_TexNormal.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelPS_Basic.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelPS_TexCoord.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelPS_TexNormal.h"
#endif

#define SSAOShadowRootIndex 5u

MainRenderPass* MainRenderPass::s_pMainRenderPass = nullptr;

MainRenderPass::MainRenderPass(std::string name, custom::RootSignature* pRootSignature, GraphicsPSO* pPSO)
	: 
	ID3D12RenderQueuePass(name, JobFactorization::ByNone), 
	m_pRootSignature(pRootSignature), 
	m_pPSO(pPSO),
	m_bAllocateRootSignature(false),
	m_bAllocatePSO(false)
{
	ASSERT(s_pMainRenderPass == nullptr);
	s_pMainRenderPass = this;

	m_SSAOShadowSRVs[0] = bufferManager::g_SSAOFullScreen.GetSRV();    // Texture2D<float>            texSSAO             : register(t64);
	m_SSAOShadowSRVs[1] = bufferManager::g_ShadowBuffer.GetSRV();      // Texture2D<float>            texShadow           : register(t65);
	m_SSAOShadowSRVs[2] = bufferManager::g_LightBuffer.GetSRV();       // StructuredBuffer<LightData> lightBuffer         : register(t66);
	m_SSAOShadowSRVs[3] = bufferManager::g_LightShadowArray.GetSRV();  // Texture2DArray<float>       lightShadowArrayTex : register(t67);
	m_SSAOShadowSRVs[4] = bufferManager::g_LightGrid.GetSRV();         // ByteAddressBuffer           lightGrid           : register(t68);
	m_SSAOShadowSRVs[5] = bufferManager::g_LightGridBitMask.GetSRV();  // ByteAddressBuffer           lightGridBitMask    : register(t69);
	
	if (m_pRootSignature == nullptr)
	{
		m_pRootSignature = new custom::RootSignature();

		m_pRootSignature->Reset(6, 2);
		m_pRootSignature->InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		m_pRootSignature->InitStaticSampler(1, premade::g_SamplerShadowDesc,  D3D12_SHADER_VISIBILITY_PIXEL);
		(*m_pRootSignature)[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b0)
		(*m_pRootSignature)[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b1)
		(*m_pRootSignature)[2].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b0)
		(*m_pRootSignature)[3].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b1)
		(*m_pRootSignature)[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0,  6, D3D12_SHADER_VISIBILITY_PIXEL); // Diffuse, Specular, Emissvie, Normal, LightMap, Reflection
		(*m_pRootSignature)[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL); // texSSAO, texShadow, lightBuffer, lightShadowArrayTex, LightGrid, LightGridMask

		m_pRootSignature->Finalize(L"InitAtMainRenderPass", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		m_bAllocateRootSignature = true;
	}
	if (m_pPSO == nullptr)
	{
		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		DXGI_FORMAT ColorFormat = bufferManager::g_SceneColorBuffer.GetFormat();
		DXGI_FORMAT DepthFormat = bufferManager::g_SceneDepthBuffer.GetFormat();

		m_pPSO = new GraphicsPSO();

		m_pPSO->SetRootSignature(*m_pRootSignature);
		m_pPSO->SetRasterizerState(premade::g_RasterizerDefault);
		m_pPSO->SetBlendState(premade::g_BlendDisable);
		m_pPSO->SetDepthStencilState(premade::g_DepthStateTestEqual);
		m_pPSO->SetInputLayout(_countof(InputElements), InputElements);
		m_pPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_pPSO->SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
		m_pPSO->SetVertexShader(g_pModelTest_VS, sizeof(g_pModelTest_VS));
		m_pPSO->SetPixelShader(g_pModelTest_DiffuseSpecularNormal_PS, sizeof(g_pModelTest_DiffuseSpecularNormal_PS));
		m_pPSO->Finalize(L"InitAtMainRenderPass");

		m_bAllocatePSO = true;
	}
}

MainRenderPass::~MainRenderPass()
{
	if (m_bAllocateRootSignature && (m_pRootSignature != nullptr))
	{
		delete m_pRootSignature;
		m_pRootSignature = nullptr;
	}

	if (m_bAllocatePSO && (m_pPSO != nullptr))
	{
		delete m_pPSO;
		m_pPSO = nullptr;
	}
}

void MainRenderPass::ExecutePass() DEBUG_EXCEPT
{
#ifdef _DEBUG
	graphics::InitDebugStartTick();
	float DeltaTime1 = graphics::GetDebugFrameTime();
#endif

	if (SSAOPass::s_pSSAOPass->m_bEnable && SSAOPass::s_pSSAOPass->m_bAsyncCompute)
	{
		custom::CommandQueue& GraphicsQueue = device::g_commandQueueManager.GetGraphicsQueue();
		custom::CommandQueue& ComputeQueue = device::g_commandQueueManager.GetComputeQueue();

		ASSERT(GraphicsQueue.WaitCommandQueueCompletedGPUSide(ComputeQueue));
	}
	if (SSAOPass::s_pSSAOPass->m_bDebugDraw)
	{
		return;
	}

	custom::GraphicsContext& graphicsContext = custom::GraphicsContext::GetRecentContext();

	uint8_t StartJobIndex = 0u;
	uint8_t MaxCommandIndex = graphicsContext.GetNumCommandLists() - 1;

	SetParams(&graphicsContext, StartJobIndex, graphicsContext.GetNumCommandLists());
	// custom::ThreadPool::EnqueueVariadic(SetParamsWithVariadic, 4u, this, &graphicsContext, StartJobIndex, MaxCommandIndex);

	graphicsContext.PIXBeginEvent(L"7_MainRenderPass", StartJobIndex);
	graphicsContext.TransitionResource(bufferManager::g_SSAOFullScreen,   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE); // UAV -> PIXEL_SHADER_RESOURCE
	graphicsContext.TransitionResource(bufferManager::g_ShadowBuffer,     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE); // 
	// graphicsContext.TransitionResource(bufferManager::g_LightBuffer,      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE); // Suboptimal Trasition
	graphicsContext.TransitionResource(bufferManager::g_LightShadowArray, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE); // ShadowPrePass->Z_PrePass���� ��.
	graphicsContext.TransitionResource(bufferManager::g_LightGrid,        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	graphicsContext.TransitionResource(bufferManager::g_LightGridBitMask, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	graphicsContext.TransitionResource(bufferManager::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ); // Suboptimal Trasition
	graphicsContext.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
	graphicsContext.SubmitResourceBarriers(StartJobIndex);

	graphicsContext.SetMainCamera(*CommandContextManager::GetMainCamera());
	graphicsContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	graphicsContext.SetViewportAndScissorRange(bufferManager::g_SceneColorBuffer, StartJobIndex, MaxCommandIndex);
	graphicsContext.SetRootSignatureRange(*m_pRootSignature, StartJobIndex, MaxCommandIndex);
	graphicsContext.SetPipelineStateRange(*m_pPSO, StartJobIndex, MaxCommandIndex);
	graphicsContext.SetRenderTargetsRange(1, &bufferManager::g_SceneColorBuffer.GetRTV(), bufferManager::g_SceneDepthBuffer.GetDSV_DepthReadOnly(), StartJobIndex, MaxCommandIndex);
	graphicsContext.SetDynamicDescriptorsRange(SSAOShadowRootIndex, 0u, _countof(m_SSAOShadowSRVs), m_SSAOShadowSRVs, StartJobIndex, MaxCommandIndex);
	graphicsContext.SetVSConstantsBufferRange(0u, StartJobIndex, MaxCommandIndex);
	graphicsContext.SetPSConstantsBufferRange(2u, StartJobIndex, MaxCommandIndex);
#ifdef _DEBUG
	custom::ThreadPool::WaitAllFinished(); // Wait Set Params.
#endif
	custom::ThreadPool::MultipleEnqueue(ContextWorkerThread, (uint8_t)(MaxCommandIndex - StartJobIndex), (void**)&m_Params[1], sizeof(RenderQueueThreadParameterWrapper));
	ContextWorkerThread(&m_Params[0]);
	custom::ThreadPool::WaitAllFinished();

	graphicsContext.TransitionResource(bufferManager::g_SSAOFullScreen,   D3D12_RESOURCE_STATE_COMMON);
	graphicsContext.TransitionResource(bufferManager::g_LightBuffer,      D3D12_RESOURCE_STATE_COMMON);
	graphicsContext.TransitionResource(bufferManager::g_LightShadowArray, D3D12_RESOURCE_STATE_COMMON);
	graphicsContext.TransitionResource(bufferManager::g_LightGrid,        D3D12_RESOURCE_STATE_COMMON);
	graphicsContext.TransitionResource(bufferManager::g_LightGridBitMask, D3D12_RESOURCE_STATE_COMMON);
	graphicsContext.SubmitResourceBarriers(MaxCommandIndex);
	graphicsContext.PIXEndEvent(MaxCommandIndex); // End SSAOPass 
	graphicsContext.Finish(false);
	
#ifdef _DEBUG
	float DeltaTime2 = graphics::GetDebugFrameTime();
	m_DeltaTime = DeltaTime2 - DeltaTime1;
#endif
}