#pragma once
// #include "stdafx.h"
#include "RawBuffer.h"

class LayoutCodex
{
public:
	static ManagedLayout Resolve(RawLayout&& layout) DEBUG_EXCEPT
	{
		auto sig = layout.GetSignature();

		auto& map = Get_().map;

		const auto i = map.find(sig);

		if (i != map.end())
		{
			layout.clearData();
			return  { i->second };
		}

		auto result = map.insert({ std::move(sig),layout.moveData() });

		return { result.first->second };
	}
private:
	static LayoutCodex& Get_() noexcept
	{
		static LayoutCodex codex;
		return codex;
	}
private:
	std::unordered_map<std::string, std::shared_ptr<IPolymorphData>> map;
};
