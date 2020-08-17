#include "stdafx.h"
#include "Sink.h"

Sink::Sink(std::string registeredNameIn)
	: registeredName(std::move(registeredNameIn))
{
	if (registeredName.empty())
	{
		ASSERT(0, "Empty output name");
	}
	const bool nameCharsValid = std::all_of(registeredName.begin(), registeredName.end(), [](char c) 
		{
		return std::isalnum(c) || c == '_';
		});
	if (!nameCharsValid || std::isdigit(registeredName.front()))
	{
		ASSERT(0, "Invalid output name : ", registeredName.c_str());
	}
}

const std::string& Sink::GetRegisteredName() const noexcept
{
	return registeredName;
}

const std::string& Sink::GetPassName() const noexcept
{
	return passName;
}

const std::string& Sink::GetOutputName() const noexcept
{
	return outputName;
}

void Sink::SetTarget(std::string passName, std::string outputName)
{
	{
		if (passName.empty())
		{
			ASSERT(0, "Empty output name.");
		}
		const bool nameCharsValid = std::all_of(passName.begin(), passName.end(), [](char c) {
			return std::isalnum(c) || c == '_';
			});
		if (passName != "$" && (!nameCharsValid || std::isdigit(passName.front())))
		{
			ASSERT(0, "Invalid output name : ", registeredName);
		}
		this->passName = passName;
	}
	{
		if (outputName.empty())
		{
			ASSERT(0, "Empty output name");
		}
		const bool nameCharsValid = std::all_of(outputName.begin(), outputName.end(), [](char c) {
			return std::isalnum(c) || c == '_';
			});
		if (!nameCharsValid || std::isdigit(outputName.front()))
		{
			ASSERT(0, "Invalid output name: ", registeredName);
		}
		this->outputName = outputName;
	}
}