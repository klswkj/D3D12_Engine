#include "stdafx.h"
#include "BufferManager.h"
#include "GPUResource.h"
#include "ColorBuffer.h"
#include "RootSignature.h"
#include "CommandContext.h"
#include "ComputeContext.h"

#include "Pass.h"
#include "BlurPass.h"
#include "RenderingResource.h"
#include "RenderTarget.h"
#include "DepthStencil.h"
#include "PremadePSO.h"
// Binding Pass + FullScreen Pass

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/OutlineWithBlurPS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/HoriontalBlurCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/VerticalBlurCS.h"
#elif !defined(_DEBUG) | defined(RELEASE)
#include "../x64/Release/Graphics(.lib)/CompiledShaders/OutlineWithBlurPS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/HoriontalBlurCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/VerticalBlurCS.h"
#endif

BlurPass::BlurPass(std::string Name, std::vector<std::shared_ptr<RenderingResource>> binds)
	: Pass(Name), m_RenderingResourcesHorizontal(std::move(binds))
{
	CalcGaussWeights(m_BlurSigma, m_BlurRadius);

	// Blur with GraphicsPSO
	m_RootSignature1.Reset(3, 1);
	m_RootSignature1[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature1[1].InitAsBufferSRV(0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature1[2].InitAsBufferUAV(0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature1.Finalize(L"Blur GraphicsRS");

	// PushBack(std::make_shared<custom::RootSignature>(m_RootSignature1));

	// Graphics Method is diused.

	m_RootSignature2.Reset(3, 1);
	m_RootSignature2[0].InitAsConstantBuffer(0);
	m_RootSignature2[1].InitAsBufferSRV(0);
	m_RootSignature2[2].InitAsBufferUAV(0);
	m_RootSignature2.Finalize(L"Blur ComputeRS");

	// PushBack(std::make_shared<custom::RootSignature>(m_RootSignature2));

#define CreatePSO( ObjName, ShaderByteCode ) \
    ObjName.SetRootSignature(m_RootSignature2); \
    ObjName.SetComputeShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
    ObjName.Finalize();

	CreatePSO(m_HorizontalComputePSO, g_pHoriontalBlurCS);
	CreatePSO(m_VerticalComputePSO, g_pVerticalBlurCS);
#undef CreatePSO
	PushBackHorizontal(std::make_shared<ComputePSO>(m_HorizontalComputePSO));
	PushBackVertical(std::make_shared<ComputePSO>(m_VerticalComputePSO));

	// m_spRenderTarget = std::make_shared<ShaderInputRenderTarget>(bufferManager::g_StencilBuffer);

	m_PingPongBuffer1.Create
	(
		L"Ping-Pong Buffer1", bufferManager::g_StencilBuffer.GetWidth(), bufferManager::g_StencilBuffer.GetHeight(), 
		1, DXGI_FORMAT_R8G8B8A8_UNORM
	);
	m_PingPongBuffer2.Create
	(
		L"Ping-Pong Buffer2", bufferManager::g_StencilBuffer.GetWidth(), bufferManager::g_StencilBuffer.GetHeight(),
		1, DXGI_FORMAT_R8G8B8A8_UNORM
	);
}

BlurPass::~BlurPass()
{
	m_PingPongBuffer1.Destroy();
	m_PingPongBuffer2.Destroy();
}

void BlurPass::PushBackHorizontal(std::shared_ptr<RenderingResource> _RenderingResource) noexcept
{
	m_RenderingResourcesHorizontal.push_back(std::move(_RenderingResource));
}

void BlurPass::PushBackVertical(std::shared_ptr<RenderingResource> _RenderingResource) noexcept
{
	m_RenderingResourcesVertical.push_back(std::move(_RenderingResource));
}

void BlurPass::finalize()
{
	Pass::finalize();
}

void BlurPass::RenderWindow()
{
	ImGui::BeginChild("Test1", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);

	bool bRadiusDirty = ImGui::SliderInt("Radius", &m_BlurRadius, 1, m_MaxRadius);
	bool bSigmaDirty = ImGui::SliderFloat("Sigma", &m_BlurSigma, 0.1f, 10.0f);

	if (bRadiusDirty | bSigmaDirty)
	{
		CalcGaussWeights(m_BlurSigma, m_BlurRadius);
	}

	ImGui::EndChild();
}

void BlurPass::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::ComputeContext& computeContext = BaseContext.GetComputeContext();

	computeContext.SetConstantArray(0, 1, &m_BlurRadius);
	computeContext.SetConstantArray(0, m_BlurRadius * 2 + 1, &m_Weights[0], 1);

	computeContext.SetRootSignature(m_RootSignature2);
	computeContext.SetPipelineState(m_HorizontalComputePSO);
	
	D3D12_RESOURCE_STATES OriginResourceState = bufferManager::g_StencilBuffer.GetResourceState();

	computeContext.TransitionResource(bufferManager::g_StencilBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
	computeContext.TransitionResource(m_PingPongBuffer1, D3D12_RESOURCE_STATE_COPY_DEST);
	computeContext.CopySubresource(m_PingPongBuffer1, 0, bufferManager::g_StencilBuffer, 0);
	
	computeContext.TransitionResource(m_PingPongBuffer1, D3D12_RESOURCE_STATE_GENERIC_READ);
	computeContext.TransitionResource(m_PingPongBuffer2, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	UINT BufferWidth = m_PingPongBuffer1.GetWidth();
	UINT BufferHeight = m_PingPongBuffer1.GetHeight();

	for (size_t i= 0; i < m_BlurCount; ++i)
	{
		// Horizontal Blur pass.

		computeContext.SetPipelineState(m_HorizontalComputePSO);

		computeContext.SetDynamicDescriptor(1, 0, m_PingPongBuffer1.GetSRV());
		computeContext.SetDynamicDescriptor(2, 0, m_PingPongBuffer2.GetUAV());

		// How many groups do we need to dispatch to cover a row of pixels, where each
		// group covers 256 pixels (the 256 is defined in the ComputeShader).
		UINT numGroupsX = (UINT)ceilf(BufferWidth / 256.0f);

		computeContext.Dispatch(numGroupsX, BufferHeight);

		computeContext.TransitionResource(m_PingPongBuffer1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(m_PingPongBuffer2, D3D12_RESOURCE_STATE_GENERIC_READ);

		// Vertical Blur pass.
		computeContext.SetPipelineState(m_VerticalComputePSO);

		computeContext.SetDynamicDescriptor(1, 0, m_PingPongBuffer2.GetSRV());
		computeContext.SetDynamicDescriptor(2, 0, m_PingPongBuffer1.GetUAV());

		// How many groups do we need to dispatch to cover a column of pixels, where each
		// group covers 256 pixels  (the 256 is defined in the ComputeShader).
		UINT numGroupsY = (UINT)ceilf(BufferHeight / 256.0f);
		computeContext.Dispatch(BufferWidth, numGroupsY);

		computeContext.TransitionResource(m_PingPongBuffer1, D3D12_RESOURCE_STATE_GENERIC_READ);
		computeContext.TransitionResource(m_PingPongBuffer2, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	computeContext.TransitionResource(bufferManager::g_StencilBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
	computeContext.TransitionResource(m_PingPongBuffer2, D3D12_RESOURCE_STATE_COPY_SOURCE);
	computeContext.CopySubresource(bufferManager::g_StencilBuffer, 0, m_PingPongBuffer2, 0);

	computeContext.TransitionResource(bufferManager::g_StencilBuffer, OriginResourceState);
}

float gauss(float x, float Sigma)
{
	const float SS = Sigma * Sigma;
	const float ReturnValue = (float)(1.0 / sqrt(2.0 * DirectX::XM_PI * SS)) * exp(-(x * x) / (2.0 * SS));
	return ReturnValue;
}

void BlurPass::CalcGaussWeights(float Sigma, UINT Radius)
{
	ASSERT(Radius < m_MaxRadius);

	const UINT Diameter = Radius * 2 + 1;

	float CoefficientsSum{ 0.0f };
	
	for (UINT i= 0; i < Diameter; ++i)
	{
		const float x = float(i - Radius);
		const float Gauss = gauss(x, Sigma);
		CoefficientsSum += Gauss;

		m_Weights[i] = Gauss;
	}

	for (size_t i= 0; i < Diameter; ++i)
	{
		m_Weights[i] /= CoefficientsSum;
	}
}
