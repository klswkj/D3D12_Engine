#include "stdafx.h"
#include "RenderTarget.h"
#include "Window.h"
#include "Device.h"

#include "DepthStencil.h"
#include "DepthBuffer.h"

void RenderTarget::BindAsBuffer(custom::CommandContext& BaseContext)
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.TransitionResource(m_SceneBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
	graphicsContext.SetRenderTarget(m_SceneBuffer.GetRTV());
}

void RenderTarget::BindAsBuffer(custom::CommandContext& BaseContext, IDisplaySurface* depthBuffer)
{
	ASSERT(dynamic_cast<DepthStencil*>(depthBuffer) != nullptr);
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.TransitionResource(m_SceneBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
	graphicsContext.TransitionResource(static_cast<DepthStencil*>(depthBuffer)->GetDepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_READ);
	graphicsContext.SetRenderTarget(m_SceneBuffer.GetRTV(), static_cast<DepthStencil*>(depthBuffer)->GetDSV());
}

void RenderTarget::Clear(custom::CommandContext& BaseContext)
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.ClearColor(m_SceneBuffer);
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTarget::GetRTV()
{
	return m_SceneBuffer.GetRTV();
}

void ShaderInputRenderTarget::Bind(custom::CommandContext& BaseContext)
{
	// TODO : 여기 RootSignature랑 PSO 언제 받아야할지 감이 안오네.
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.TransitionResource(m_SceneBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	graphicsContext.SetDynamicDescriptor((m_IndexBitMap >> 6) & 0x3f, m_IndexBitMap & 0x3f, m_SceneBuffer.GetSRV());
}

void OutputOnlyRenderTarget::Bind(custom::CommandContext& BaseContext)
{
	ASSERT(false, "Cannot Bind RenderTarget.");
}