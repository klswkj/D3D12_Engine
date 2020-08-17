#include "stdafx.h"
#include "Pass.h"
#include "Sink.h"
#include "Source.h"

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
	for (auto& inputSink : m_Sinks)
	{
		inputSink->PostLinkValidate();
	}
}

const std::vector<std::unique_ptr<Sink>>& Pass::GetSinks() const
{
	return m_Sinks;
}

Source& Pass::GetSource(const std::string& Name) const
{
	for (auto& source : m_Sources)
	{
		if (source->GetRegisteredName() == Name)
		{
			return *source;
		}
	}

	ASSERT(0, "Output named [", Name, "] not found in Pass : ", GetRegisteredName());
}

Sink& Pass::GetSink(const std::string& RegisteredName) const
{
	for (auto& sink : m_Sinks)
	{
		if (sink->GetRegisteredName() == RegisteredName)
		{
			return *sink;
		}
	}

	ASSERT(0, "Input named [", RegisteredName, "] not found in Pass : ", GetRegisteredName());
}

void Pass::PushBackSink(std::unique_ptr<Sink> spSink)
{
	const std::string& ToRegisterName = spSink->GetRegisteredName();
	
	// check for overlap of input names
	for (auto& si : m_Sinks)
	{
		if (si->GetRegisteredName() == ToRegisterName)
		{
			ASSERT(0, "Registered input overlaps with existing: ", spSink->GetRegisteredName());
		}
	}

	m_Sinks.push_back(std::move(spSink));
}

void Pass::PushBackSource(std::unique_ptr<Source> spSource)
{
	const std::string& ToRegisterName = spSource->GetRegisteredName();
	// check for overlap of output names
	for (auto& source : m_Sources)
	{
		if (source->GetRegisteredName() == spSource->GetRegisteredName())
		{
			ASSERT(0, "Registered output overlaps with existing : ", spSource->GetRegisteredName().c_str());
		}
	}

	m_Sources.push_back(std::move(spSource));
}

void Pass::SetSinkLinkage(const std::string& RegisteredName, const std::string& Target)
{
	std::vector<std::string> targetSplit = SplitString(Target, ".");

	if (targetSplit.size() != 2ul)
	{
		ASSERT(0, "Input target has incorrect format");
	}

	Sink& sink = GetSink(RegisteredName);
	sink.SetTarget(std::move(targetSplit[0]), std::move(targetSplit[1]));
}