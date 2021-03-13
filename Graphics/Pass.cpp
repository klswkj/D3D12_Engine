#include "stdafx.h"
#include "Pass.h"

static size_t passIndex = 0;

D3D12Pass::D3D12Pass(std::string Name) DEBUG_EXCEPT
	: 
	m_Name(Name),
	m_bActive(true),
	m_PassIndex(passIndex++)
{
	m_AttributeFlags =
	{
		ePassShadingTechniqueFlags::eForwardRenderingPass,
		ePassGPUAsyncFlags::eSingleGPUAsync,
		ePassStageFlags::eMainProcessing
	};
}

D3D12Pass::~D3D12Pass()
{
}

void D3D12Pass::Reset() DEBUG_EXCEPT
{
}

void D3D12Pass::RenderWindow() DEBUG_EXCEPT
{
}

std::string D3D12Pass::GetRegisteredName() const DEBUG_EXCEPT
{
	return m_Name;
}