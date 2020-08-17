#include "stdafx.h"
#include "Source.h"

Source::Source(std::string nameIn)
	: name(std::move(nameIn))
{
	if (name.empty())
	{
		ASSERT(0, "Empty output name.");
	}
	const bool nameCharsValid = std::all_of(name.begin(), name.end(), [](char c) {
		return std::isalnum(c) || c == '_';
		});
	if (!nameCharsValid || std::isdigit(name.front()))
	{
		ASSERT(0, "Invalid output name : ", name);
	}
}

std::shared_ptr<Bindable> Source::YieldBindable()
{
	ASSERT(0, "Output cannot be accessed as bindable.");
}

std::shared_ptr<IDisplaySurface> Source::YieldBuffer()
{
	ASSERT(0, "Output cannot be accessed as buffer");
}

const std::string& Source::GetRegisteredName() const noexcept
{
	return name;
}
