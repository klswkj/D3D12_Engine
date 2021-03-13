#pragma once
#include "stdafx.h"
#include "IDisplaySurface.h"
#include "RenderingResource.h"

class ColorBuffer;

namespace custom
{
	class CommandContext;
	class RootSignature;
	class PSO;
}

class RenderTarget : public IDisplaySurface, public RenderingResource
{
public:
	void BindAsBuffer(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void BindAsBuffer(custom::CommandContext& BaseContext, IDisplaySurface* depthBuffer) DEBUG_EXCEPT override;
	void Clear(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;

	ColorBuffer& GetSceneBuffer() { return m_SceneBuffer; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV();

protected:
	RenderTarget(ColorBuffer& colorBuffer) // g_SceneColorBuffer
		: m_SceneBuffer(colorBuffer) 
	{
	}

protected:
	ColorBuffer& m_SceneBuffer; // or m_RenderTarget;
};

class ShaderInputRenderTarget : public RenderTarget
{
public:
	// using RenderTarget::RenderTarget;
	ShaderInputRenderTarget(ColorBuffer& colorBuffer, custom::RootSignature& RS, custom::PSO& PSO, UINT RootParameterIndex, UINT ShaderRegisterIndex)
		: RenderTarget(colorBuffer), m_RootSignature(RS), m_PSO(PSO)
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
	uint16_t m_IndexBitMap= 0;
};

// RT for Graphics to create RenderTarget for the back buffer
class OutputOnlyRenderTarget : public RenderTarget
{
public:
	using RenderTarget::RenderTarget;
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
};

