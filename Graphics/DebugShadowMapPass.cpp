#include "stdafx.h"
#include "DebugShadowMapPass.h"
#include "premadePSO.h"
#include "BufferManager.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DebugShadowMap_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DebugShadowMap_PS.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/Release/Graphics(.lib)/CompiledShaders/DebugShadowMap_VS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/DebugShadowMap_PS.h"
#endif

DebugShadowMapPass* DebugShadowMapPass::s_pDebugWireFramePass = nullptr;

DebugShadowMapPass::DebugShadowMapPass(std::string Name)
	: D3D12Pass(Name)
{
	ASSERT(s_pDebugWireFramePass == nullptr);
	s_pDebugWireFramePass = this;

	LONG Width  = (LONG)bufferManager::g_ShadowBuffer.GetWidth();
	LONG Height = (LONG)bufferManager::g_ShadowBuffer.GetHeight();

	m_Topology     = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_ViewPort     = { 0.0f, 0.0f, (float)Width, (float)Height, 0.0f, 0.0f };
	m_ScissorRect  = { 0l, 0l, Width, Height };

	m_RootSignature.Reset(1, 1);
	m_RootSignature[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature.InitStaticSampler(0, premade::g_SamplerLinearWrapDesc);
	m_RootSignature.Finalize(L"DebugShadowMap_RS");

	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetRasterizerState(premade::g_RasterizerTwoSided);
	m_PSO.SetBlendState(premade::g_BlendDisable);
	m_PSO.SetDepthStencilState(premade::g_DepthStateDisabled);
	m_PSO.SetSampleMask(0xFFFFFFFF);
	m_PSO.SetInputLayout(0, nullptr);
	m_PSO.SetPrimitiveTopologyType(m_TopologyType);
	m_PSO.SetVertexShader(g_pDebugShadowMap_VS, sizeof(g_pDebugShadowMap_VS));
	m_PSO.SetPixelShader(g_pDebugShadowMap_PS, sizeof(g_pDebugShadowMap_PS));
	m_PSO.SetRenderTargetFormat(bufferManager::g_SceneColorBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
	m_PSO.Finalize(L"DebugShadowMap_PSO");
}

DebugShadowMapPass::~DebugShadowMapPass()
{
}

void DebugShadowMapPass::ExecutePass() DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = custom::GraphicsContext::Begin(1);
	
	// RenderTarget  : ColorBuffer
	// Drawing Stuff : ShadowBuffer
	graphicsContext.PIXBeginEvent(L"DebugShadowMapPass", 0u);
	graphicsContext.TransitionResource(bufferManager::g_ShadowBuffer,     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0U);
	graphicsContext.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, 0U);
	graphicsContext.SubmitResourceBarriers(0u);
	graphicsContext.SetRenderTarget(bufferManager::g_SceneColorBuffer.GetRTV(), 0u);
	graphicsContext.SetRootSignature(m_RootSignature, 0u);
	graphicsContext.SetPipelineState(m_PSO, 0u);
	graphicsContext.SetDynamicDescriptor(0, 0, bufferManager::g_ShadowBuffer.GetSRV(), 0u);
	graphicsContext.Draw(6, 0u);
	graphicsContext.PIXEndEvent(0u);
	graphicsContext.Finish(false);
}