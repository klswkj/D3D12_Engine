#include "stdafx.h"
#include "DebugWireFramePass.h"
#include "CommandContext.h"

DebugWireFramePass* DebugWireFramePass::s_pDebugWireFramePass = nullptr;

DebugWireFramePass::DebugWireFramePass(std::string Name)
	: ID3D12RenderQueuePass(Name, JobFactorization::ByEntity)
{
	ASSERT(s_pDebugWireFramePass == nullptr);
	s_pDebugWireFramePass = nullptr;


}

void DebugWireFramePass::ExecutePass() DEBUG_EXCEPT
{


}

/*
#include "stdafx.h"
#include "TestPass.h"
#include "CommandContext.h"
#include "ComputeContext.h"

#include "BufferManager.h"
#include "PreMadePSO.h"
#include "Camera.h"
// 헤더 4x4 projViewMatrix만 받고 그리는 쉐이더 헤더파일
#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_PS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DebugCS.h"

#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_DiffuseSpecularNormal_PS.h"
#endif

TestPass* TestPass::s_pMainRenderPass = nullptr;

TestPass::TestPass(std::string Name)
	: ID3D12RenderQueuePass(Name)
{
	ASSERT(s_pMainRenderPass == nullptr);
	s_pMainRenderPass = nullptr;

	{
		// RootSignature
		
		m_RootSignature.Reset(2, 0);
		m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
		m_RootSignature[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);
		m_RootSignature.Finalize(L"TestPass Graphics RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		
		
		m_RootSignature.Reset(5, 2);
		m_RootSignature.InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		m_RootSignature.InitStaticSampler(1, premade::g_SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);                                      // vsConstants(b0)
		m_RootSignature[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);                                       // psConstants(b0)
		m_RootSignature[2].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);                                       // psConstants(b1)
		m_RootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL); // Diffuse, Specular, Emissvie, Normal, LightMap, Reflection
		m_RootSignature[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL); // texSSAO, texShadow, lightBuffer, lightShadowArrayTex, LightGrid, LightGridMask
		m_RootSignature.Finalize(L"InitAtTestPass", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		
		// PSO
		D3D12_INPUT_ELEMENT_DESC vertexElements[] =
		{
			{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		DXGI_FORMAT ColorFormat = bufferManager::g_SceneDebugBuffer.GetFormat();
		DXGI_FORMAT DepthFormat = bufferManager::g_SceneDepthBuffer.GetFormat();

		m_PSO.SetRootSignature        (m_RootSignature);
		m_PSO.SetRasterizerState      (premade::g_WireFrameDefault);
		m_PSO.SetBlendState           (premade::g_BlendNoColorWrite);
		m_PSO.SetDepthStencilState    (premade::g_DepthStateDisabled);
		m_PSO.SetInputLayout          (_countof(vertexElements), vertexElements);
		m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_PSO.SetRenderTargetFormats  (1, &ColorFormat, DepthFormat);
		// m_PSO.SetVertexShader         (g_pFlat_VS, sizeof(g_pFlat_VS));
		// m_PSO.SetPixelShader          (g_pFlat_PS, sizeof(g_pFlat_PS));
		m_PSO.SetVertexShader(g_pModelTest_VS, sizeof(g_pModelTest_VS));
		m_PSO.SetPixelShader(g_pModelTest_DiffuseSpecularNormal_PS, sizeof(g_pModelTest_DiffuseSpecularNormal_PS));
		m_PSO.Finalize(L"TestPass Graphics PSO");
	}
	{
		m_CopyRootSignature.Reset(2, 2);
		m_CopyRootSignature.InitStaticSampler(0, premade::g_SamplerLinearClampDesc);
		m_CopyRootSignature.InitStaticSampler(1, premade::g_SamplerLinearBorderDesc);
		m_CopyRootSignature[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
		m_CopyRootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
		m_CopyRootSignature.Finalize(L"TestPass Compute RS");

		m_CopyPSO.SetRootSignature(m_CopyRootSignature);
		m_CopyPSO.SetComputeShader(g_pDebugCS, sizeof(g_pDebugCS));
		m_CopyPSO.Finalize(L"TestPass Compute PSO");
	}

	m_Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}
void TestPass::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	BaseContext.PIXBeginEvent(L"TestPass");

	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.PIXBeginEvent(L"Draw WireMode");

	graphicsContext.SetRootSignature(m_RootSignature);
	graphicsContext.SetPipelineState(m_PSO);
	graphicsContext.SetPrimitiveTopology(m_Topology);

	graphicsContext.TransitionResource(bufferManager::g_SceneDebugBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	graphicsContext.ClearColor(bufferManager::g_SceneDebugBuffer);
	graphicsContext.SetRenderTarget(bufferManager::g_SceneDebugBuffer.GetRTV());
	graphicsContext.SetViewport(D3D12_VIEWPORT{ 0.0f, 0.0f, (float)bufferManager::g_SceneDebugBuffer.GetWidth(), (float)bufferManager::g_SceneDebugBuffer.GetHeight(), 0.0f, 1.0f });

	graphicsContext.SetDynamicConstantBufferView(0, sizeof(Math::Matrix4), &graphicsContext.GetpMainCamera()->GetViewProjMatrix());
	graphicsContext.SetDynamicConstantBufferView(1, sizeof(m_LineColor), &m_LineColor);
	// graphicsContext.SetDynamicConstantBufferView(2, sizeof(m_LineColor), &m_LineColor);
	
	// graphicsContext.SetMainCamera(*graphicsContext.GetpMainCamera());
	// graphicsContext.SetVSConstantsBuffer(0);
	// graphicsContext.SetPSConstantsBuffer(1);

	ID3D12RenderQueuePass::Execute(BaseContext);

	graphicsContext.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	graphicsContext.ClearColor(bufferManager::g_SceneColorBuffer);

	graphicsContext.PIXEndEvent();

	custom::ComputeContext& CC = graphicsContext.GetComputeContext();
	CC.PIXBeginEvent(L"Copy DebugBuffer->ColorBuffer");
	CC.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	CC.TransitionResource(bufferManager::g_SceneDebugBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	CC.SetRootSignature(m_CopyRootSignature);
	CC.SetPipelineState(m_CopyPSO);
	CC.SetDynamicDescriptor(0, 0, bufferManager::g_SceneColorBuffer.GetUAV());
	CC.SetDynamicDescriptor(1, 0, bufferManager::g_SceneDebugBuffer.GetSRV());
	CC.Dispatch2D(bufferManager::g_SceneDebugBuffer.GetWidth(), bufferManager::g_SceneDebugBuffer.GetHeight());
	
	CC.PIXEndEvent();

	BaseContext.PIXEndEvent(); // End TestPass
}
*/