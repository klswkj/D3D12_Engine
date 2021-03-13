#pragma once
#include "RenderingResource.h"

namespace custom
{
	class CommandContext;
}

class FundamentalTexture : public RenderingResource
{
public:
	FundamentalTexture(const char* szName, UINT RootIndex, UINT ShaderOffset, D3D12_CPU_DESCRIPTOR_HANDLE SRV);
	void Bind(custom::CommandContext& BaseContext, uint8_t commandIndex) DEBUG_EXCEPT override;

private:
#ifdef _DEBUG
	char m_Name[128];
#endif
	UINT m_RootParameterIndex = -1;
	UINT m_ShaderOffset = -1;
	D3D12_CPU_DESCRIPTOR_HANDLE m_TextureSRV;
};