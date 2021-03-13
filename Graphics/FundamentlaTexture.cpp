#include "stdafx.h"
#include "FundamentalTexture.h"
// #include "RenderingResource.h"
#include "CommandContext.h"

FundamentalTexture::FundamentalTexture(const char* szName, UINT RootIndex, UINT ShaderOffset, D3D12_CPU_DESCRIPTOR_HANDLE SRV)
	: m_RootParameterIndex(RootIndex), m_ShaderOffset(ShaderOffset), m_TextureSRV(SRV)
{
#ifdef _DEBUG
	ASSERT(!strncpy_s(m_Name, szName, 127));
#endif
}

void FundamentalTexture::Bind(custom::CommandContext& BaseContext, uint8_t commandIndex) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.SetDynamicDescriptor(m_RootParameterIndex, m_ShaderOffset, m_TextureSRV, commandIndex);
}