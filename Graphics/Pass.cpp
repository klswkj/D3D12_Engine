#include "stdafx.h"
#include "Pass.h"

static uint32_t passIndex = 0;

Pass::Pass(std::string Name) noexcept
	: m_Name(Name), m_PassIndex(passIndex++)
{
}

Pass::~Pass()
{
}

void Pass::Reset() DEBUG_EXCEPT
{
}

void Pass::RenderWindow() DEBUG_EXCEPT
{
}

std::string Pass::GetRegisteredName() const noexcept
{
	return m_Name;
}

void Pass::finalize()
{
	// Still Empty.
}