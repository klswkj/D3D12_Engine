#pragma once
#include "IDisplaySurface.h"
#include "RenderingResource.h"
#include "DepthBuffer.h"

class ShadowBuffer;

class DepthStencil : public IDisplaySurface, public RenderingResource
{
public:
	void BindAsBuffer(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void BindAsBuffer(custom::CommandContext& BaseContext, IDisplaySurface* RenderTarget) DEBUG_EXCEPT override;
	void Clear(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;

	DepthBuffer& GetDepthBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV();

protected:
	DepthStencil(ShadowBuffer& shadowBuffer);

protected:
	ShadowBuffer& m_DepthBuffer;
};

class ShaderInputDepthStencil : public DepthStencil
{
public:
	ShaderInputDepthStencil(ShadowBuffer& shadowBuffer, custom::RootSignature& RS, custom::PSO& PSO, UINT RootParameterIndex, UINT ShaderRegisterIndex)
		: DepthStencil(shadowBuffer), m_RootSignature(RS), m_PSO(PSO)
	{
		ASSERT(64u <= RootParameterIndex && 64u <= ShaderRegisterIndex, "Exceeded Limit.");
		m_IndexBitMap |= RootParameterIndex << 6;
		m_IndexBitMap |= ShaderRegisterIndex;
	}

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;

	void ResetParameter(UINT RootParameterIndex, UINT ShaderRegisterIndex)
	{
		ASSERT(64u <= RootParameterIndex && 64u <= ShaderRegisterIndex, "Exceeded Limit.");
		m_IndexBitMap = 0;
		m_IndexBitMap |= RootParameterIndex << 6;
		m_IndexBitMap |= ShaderRegisterIndex;
	}
private:
	custom::RootSignature& m_RootSignature;
	custom::PSO& m_PSO;
	uint16_t m_IndexBitMap{ 0 };
};

class OutputOnlyDepthStencil : public DepthStencil
{
public:
	using DepthStencil::DepthStencil;
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
};

