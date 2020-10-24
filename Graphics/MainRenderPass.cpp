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
#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelVS_Basic.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelVS_TexCoord.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelVS_TexNormal.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelPS_Basic.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelPS_TexCoord.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelPS_TexNormal.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelVS_Basic.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelVS_TexCoord.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelVS_TexNormal.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelPS_Basic.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelPS_TexCoord.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelPS_TexNormal.h"
#endif

MainRenderPass::MainRenderPass(std::string Name)
	: RenderQueuePass(Name)
{
	m_SSAOShadowSRVs[0] = bufferManager::g_SSAOFullScreen.GetSRV();              // Texture2D<float>            texSSAO             : register(t64);
	m_SSAOShadowSRVs[1] = bufferManager::g_ShadowBuffer.GetSRV();                // Texture2D<float>            texShadow           : register(t65);
	m_SSAOShadowSRVs[2] = bufferManager::g_LightBuffer.GetSRV();       // StructuredBuffer<LightData> lightBuffer         : register(t66);
	m_SSAOShadowSRVs[3] = bufferManager::g_LightShadowArray.GetSRV();  // Texture2DArray<float>       lightShadowArrayTex : register(t67);
	m_SSAOShadowSRVs[4] = bufferManager::g_LightGrid.GetSRV();         // ByteAddressBuffer           lightGrid           : register(t68);
	m_SSAOShadowSRVs[5] = bufferManager::g_LightGridBitMask.GetSRV();  // ByteAddressBuffer           lightGridBitMask    : register(t69);
}

MainRenderPass::~MainRenderPass()
{
}

void MainRenderPass::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
#ifdef _DEBUG
	graphics::InitDebugStartTick();
	float DeltaTime1 = graphics::GetDebugFrameTime();
#endif

	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.PIXBeginEvent(L"7_MainRenderPass");

	// If SSAOPass::bEnabled && SSAOPass::bSync
	{
		graphicsContext.Flush();
		// Make the 3D queue wait for the Compute queue to finish SSAO
		ASSERT(device::g_commandQueueManager.GetGraphicsQueue().StallForProducer(device::g_commandQueueManager.GetComputeQueue()));

		// Must Set pfnSetupGraphicsState();
	}

	graphicsContext.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	graphicsContext.TransitionResource(bufferManager::g_SSAOFullScreen, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	graphicsContext.TransitionResource(bufferManager::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);

	graphicsContext.SetRenderTarget(bufferManager::g_SceneColorBuffer.GetRTV(), bufferManager::g_SceneDepthBuffer.GetDSV_DepthReadOnly());
	graphicsContext.SetDynamicDescriptors(3, 0, _countof(m_SSAOShadowSRVs), m_SSAOShadowSRVs);
	graphicsContext.SetVSConstantsBuffer(0u);
	graphicsContext.SetPSConstantsBuffer(1u);

	RenderQueuePass::Execute(BaseContext);

	graphicsContext.PIXEndEvent();
#ifdef _DEBUG
	float DeltaTime2 = graphics::GetDebugFrameTime();
	m_DeltaTime = DeltaTime2 - DeltaTime1;
#endif
}

void MainRenderPass::Reset() DEBUG_EXCEPT
{

}

/*
MainRenderPass::MainRenderPass(std::string Name, custom::RootSignature* pRootSignature, GraphicsPSO* pMainRenderPSO/)
	: RenderQueuePass(Name), m_pRootSignature(pRootSignature), m_pMainRenderPSO(pMainRenderPSO)
{
	if (m_pRootSignature == nullptr)
	{
		m_pRootSignature = new custom::RootSignature();

		m_pRootSignature->Reset(4, 2);
		m_pRootSignature->InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);                // 
		m_pRootSignature->InitStaticSampler(1, premade::g_SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);                 //
		(*m_pRootSignature)[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b0)                   // 
		(*m_pRootSignature)[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b1)                   //               // 
		(*m_pRootSignature)[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);  // 
		(*m_pRootSignature)[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL); // 

		m_pRootSignature->Finalize(L"InitAtMainRenderPass", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		m_bAllocateRootSignature = true;
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

		ASSERT(InputElements[1].Format == DXGI_FORMAT_R32G32_FLOAT);

		m_pMainRenderPSO->SetRootSignature(*m_pRootSignature);
		m_pMainRenderPSO->SetRasterizerState(premade::g_RasterizerDefault);
		m_pMainRenderPSO->SetBlendState(premade::g_BlendDisable);
		m_pMainRenderPSO->SetDepthStencilState(premade::g_DepthStateTestEqual);
		m_pMainRenderPSO->SetInputLayout(_countof(InputElements), InputElements);
		m_pMainRenderPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_pMainRenderPSO->SetVertexShader(g_pModelVS_TexNormal, sizeof(g_pModelVS_TexNormal));
		m_pMainRenderPSO->SetPixelShader(g_pModelPS_TexNormal, sizeof(g_pModelPS_TexNormal));
		m_pMainRenderPSO->SetRenderTargetFormats(1, &bufferManager::g_SceneColorBuffer.GetFormat(), bufferManager::g_SceneDepthBuffer.GetFormat());
		m_pMainRenderPSO->Finalize();

		m_bAllocatePSO = true;
	}

	m_SSAOShadowSRVs[0] = bufferManager::g_SSAOFullScreen.GetSRV();              // Texture2D<float>            texSSAO             : register(t64);
	m_SSAOShadowSRVs[1] = bufferManager::g_ShadowBuffer.GetSRV();                // Texture2D<float>            texShadow           : register(t65);
	m_SSAOShadowSRVs[2] = bufferManager::g_LightBuffer.GetSRV();       // StructuredBuffer<LightData> lightBuffer         : register(t66);
	m_SSAOShadowSRVs[3] = bufferManager::g_LightShadowArray.GetSRV();  // Texture2DArray<float>       lightShadowArrayTex : register(t67);
	m_SSAOShadowSRVs[4] = bufferManager::g_LightGrid.GetSRV();         // ByteAddressBuffer           lightGrid           : register(t68);
	m_SSAOShadowSRVs[5] = bufferManager::g_LightGridBitMask.GetSRV();  // ByteAddressBuffer           lightGridBitMask    : register(t69);
} 
*/

/*

MainRenderPass::~MainRenderPass()
{
	if (m_bAllocateRootSignature && m_pRootSignature != nullptr)
	{
		delete m_pRootSignature;
		m_pRootSignature = nullptr;
	}

	if (m_bAllocatePSO && m_pMainRenderPSO != nullptr)
	{
		delete m_pMainRenderPSO;
		m_pMainRenderPSO = nullptr;
	}
}
*/

/*
void MainRenderPass::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	// If SSAOPass::bEnabled && SSAOPass::bSync
	{
		graphicsContext.Flush();
		// Make the 3D queue wait for the Compute queue to finish SSAO
		ASSERT(device::g_commandQueueManager.GetGraphicsQueue().StallForProducer(device::g_commandQueueManager.GetComputeQueue()));

		// Must Set pfnSetupGraphicsState();
	}

	graphicsContext.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	graphicsContext.TransitionResource(bufferManager::g_SSAOFullScreen, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	graphicsContext.TransitionResource(bufferManager::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);

	// graphicsContext.SetDynamicDescriptors(3, 0, _countof(m_SSAOShadowSRVs), m_SSAOShadowSRVs);
	graphicsContext.SetDynamicDescriptors(3, 0, _countof(m_SSAOShadowSRVs), m_SSAOShadowSRVs);
	graphicsContext.SetPipelineState(*m_pMainRenderPSO);

	graphicsContext.SetRenderTarget(bufferManager::g_SceneColorBuffer.GetRTV(), bufferManager::g_SceneDepthBuffer.GetDSV_DepthReadOnly());

	RenderQueuePass::Execute(BaseContext);
}
*/