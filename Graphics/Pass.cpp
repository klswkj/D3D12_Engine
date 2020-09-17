#include "stdafx.h"
#include "Pass.h"

static int32_t passIndex{ 0 };

Pass::Pass(const char* Name) noexcept
	: m_Name(Name), m_PassIndex(passIndex++)
{
}

Pass::~Pass()
{
}

void Pass::Reset() DEBUG_EXCEPT
{
}

const char* Pass::GetRegisteredName() const noexcept
{
	return m_Name;
}

void Pass::finalize()
{
	// Still Empty.
}