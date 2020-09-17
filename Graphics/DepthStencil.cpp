#include "stdafx.h"
#include "DepthStencil.h"
#include "DepthBuffer.h"

#include "ShadowBuffer.h"
#include "RenderTarget.h"
#include "Matrix4.h"
#include "ShaderConstantsTypeDefinitions.h"
#include "ObjectFilterFlag.h"

DepthStencil::DepthStencil(ShadowBuffer& shadowBuffer)
	: m_DepthBuffer(shadowBuffer) {}

void DepthStencil::BindAsBuffer(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.TransitionResource(m_DepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
	graphicsContext.SetOnlyDepthStencil(GetDSV());
}

void DepthStencil::BindAsBuffer(custom::CommandContext& BaseContext, IDisplaySurface* _RenderTarget) DEBUG_EXCEPT
{
	ASSERT(dynamic_cast<RenderTarget*>(_RenderTarget) != nullptr);
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.TransitionResource(m_DepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
	graphicsContext.TransitionResource(static_cast<RenderTarget*>(_RenderTarget)->GetSceneBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	graphicsContext.SetRenderTarget(static_cast<RenderTarget*>(_RenderTarget)->GetRTV(), GetDSV());
}

void DepthStencil::Clear(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.ClearDepth(m_DepthBuffer);
}

DepthBuffer& DepthStencil::GetDepthBuffer() 
{ 
	return m_DepthBuffer; 
}

const D3D12_CPU_DESCRIPTOR_HANDLE& DepthStencil::GetDSV()
{ 
	return m_DepthBuffer.GetDSV_DepthReadOnly(); 
}

void ShaderInputDepthStencil::SetMatrix(Math::Matrix4& ViewProjMatrix)
{
	m_ViewProjMatrix = ViewProjMatrix;
}

void ShaderInputDepthStencil::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	VSConstants vsConstants;
	vsConstants.modelToProjection = m_ViewProjMatrix;                   // TODO : 
	// vsConstants.modelToShadow = m_SunShadow.GetShadowMatrix();       !!!!!!!!!!!!!!!!!!!!!!!!!!!! ShadowCamera 어떻게???????
	// XMStoreFloat3(&vsConstants.viewerPos, m_Camera.GetPosition());   !!!!!!!!!!!!!!!!!!!!!!!!!!!! CameraPosition 어떻게?????

	graphicsContext.SetDynamicConstantBufferView(0, sizeof(vsConstants), &vsConstants);
}

void OutputOnlyDepthStencil::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	ASSERT(false, "Cannot Bind DepthStencil.");
}