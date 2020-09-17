#pragma once
#include <vector>
#include <string>
#include <memory.h>
#include <ppl.h>
#include <ppltasks.h>

#if !defined byte
#define byte unsigned char
#endif

namespace fileReader
{
	extern std::shared_ptr<std::vector<byte>> NullFile;

	std::shared_ptr<std::vector<byte>> ReadFileSync(const std::wstring& fileName);

	// typedef Concurrency::task<std::shared_ptr<std::vector<byte>>> ResultType;
	Concurrency::task<std::shared_ptr<std::vector<byte>>> ReadFileAsync(const std::wstring& fileName);
} // namespace fileReader