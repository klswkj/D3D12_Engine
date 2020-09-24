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

MainRenderPass::MainRenderPass(std::string Name, custom::RootSignature* pRootSignature/* = nullptr*/, GraphicsPSO* pMainRenderPSO/* = nullptr*/)
	: RenderQueuePass(Name), m_pRootSignature(pRootSignature), m_pMainRenderPSO(pMainRenderPSO)
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

		ASSERT(InputElements[1].Format == DXGI_FORMAT_R32G32_FLOAT);

		m_pMainRenderPSO->SetRootSignature(*m_pRootSignature);
		m_pMainRenderPSO->SetRasterizerState(premade::g_RasterizerDefault);
		m_pMainRenderPSO->SetBlendState(premade::g_BlendDisable);
		m_pMainRenderPSO->SetDepthStencilState(premade::g_DepthStateTestEqual);
		m_pMainRenderPSO->SetInputLayout(_countof(InputElements), InputElements);
		m_pMainRenderPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_pMainRenderPSO->SetVertexShader(g_pModelVS_TexNormal, sizeof(g_pModelVS_TexNormal));
		m_pMainRenderPSO->SetPixelShader(g_pModelPS_TexNormal, sizeof(g_pModelPS_TexNormal));
		m_pMainRenderPSO->SetRenderTargetFormats(1, &bufferManager::g_SceneColorBuffer.GetFormat(), bufferManager::g_ShadowBuffer.GetFormat());
		m_pMainRenderPSO->Finalize();
	}

	m_SSAOShadowSRVs[0] = bufferManager::g_SSAOFullScreen.GetSRV();              // Texture2D<float>            texSSAO             : register(t64);
	m_SSAOShadowSRVs[1] = bufferManager::g_ShadowBuffer.GetSRV();                // Texture2D<float>            texShadow           : register(t65);
	m_SSAOShadowSRVs[2] = bufferManager::g_LightBuffer.GetSRV();       // StructuredBuffer<LightData> lightBuffer         : register(t66);
	m_SSAOShadowSRVs[3] = bufferManager::g_LightShadowArray.GetSRV();  // Texture2DArray<float>       lightShadowArrayTex : register(t67);
	m_SSAOShadowSRVs[4] = bufferManager::g_LightGrid.GetSRV();         // ByteAddressBuffer           lightGrid           : register(t68);
	m_SSAOShadowSRVs[5] = bufferManager::g_LightGridBitMask.GetSRV();  // ByteAddressBuffer           lightGridBitMask    : register(t69);
}

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

	graphicsContext.SetDynamicDescriptors(3, 0, _countof(m_SSAOShadowSRVs), m_SSAOShadowSRVs);
	graphicsContext.SetPipelineState(*m_pMainRenderPSO);
	graphicsContext.SetRenderTarget(bufferManager::g_SceneColorBuffer.GetRTV(), bufferManager::g_SceneDepthBuffer.GetDSV_DepthReadOnly());
	// graphicsContext.SetViewportAndScissor(m_pMainViewport, m_pMainScissor);
	// MainViewport, MainScissor at MasterRenderGraph -> 

	RenderQueuePass::Execute(BaseContext);
}

void MainRenderPass::Reset() DEBUG_EXCEPT
{

}


/*
D3D12 ERROR: ID3D12CommandQueue::ExecuteCommandLists: Using ClearRenderTargetView on Command List 
(0x000001E8BB07E060:'CommandQueueManager.cpp, Function : CommandQueueManager::CreateNewCommandList, Line : 57')
: Resource state (0x0: D3D12_RESOURCE_STATE_[COMMON|PRESENT]) of resource (0x000001E8AA173140:'Main Color Buffer') 
(subresource: 0) is invalid for use as a render target.  
Expected State Bits (all): 0x4: D3D12_RESOURCE_STATE_RENDER_TARGET, Actual State: 0x0: D3D12_RESOURCE_STATE_[COMMON|PRESENT], 
Missing State: 0x4: D3D12_RESOURCE_STATE_RENDER_TARGET. [ EXECUTION ERROR #538: INVALID_SUBRESOURCE_STATE]


D3D12 ERROR: ID3D12Device::CopyDescriptors: Specified CPU descriptor handle ptr=0xFFFFFFFFFFFFFFFF does not refer to a location in a descriptor heap. 
pSrcDescriptorRangeStarts[2] is the issue. [ EXECUTION ERROR #646: INVALID_DESCRIPTOR_HANDLE]

Exception thrown at 0x00007FFC5BC43B29 (KernelBase.dll) in Graphics(.lib).exe: 0x0000087D (parameters: 0x0000000000000000, 0x000000E7413CB030, 0x000001E8A72DFFB0).
Unhandled exception at 0x00007FFC5BC43B29 (KernelBase.dll) in Graphics(.lib).exe: 0x0000087D (parameters: 0x0000000000000000, 0x000000E7413CB030, 0x000001E8A72DFFB0).

Unhandled exception at 0x00007FFC5BC43B29 (KernelBase.dll) in Graphics(.lib).exe: 0x0000087D (parameters: 0x0000000000000000, 0x0000007779B9AA40, 0x000001AA49497A70).
*/