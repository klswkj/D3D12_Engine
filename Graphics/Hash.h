#pragma once
#include <stdint.h>
#include <intrin.h>

#ifdef _M_X64
#define ENABLE_SSE_CRC32 1
#else
#define ENABLE_SSE_CRC32 0
#endif

#if ENABLE_SSE_CRC32
#pragma intrinsic(_mm_crc32_u32)
#pragma intrinsic(_mm_crc32_u64)
#endif

namespace HashInternal
{
	template <typename T> __forceinline T AlignUpWithMask(T value, size_t mask)
	{
		return (T)(((size_t)value + mask) & ~mask);
	}

	template <typename T> __forceinline T AlignDownWithMask(T value, size_t mask)
	{
		return (T)((size_t)value & ~mask);
	}

	template <typename T> __forceinline T AlignUp(T value, size_t alignment)
	{
		return AlignUpWithMask(value, alignment - 1);
	}

	template <typename T> __forceinline T AlignDown(T value, size_t alignment)
	{
		return AlignDownWithMask(value, alignment - 1);
	}

	template <typename T> __forceinline bool IsAligned(T value, size_t alignment)
	{
		return 0 == ((size_t)value & (alignment - 1));
	}

	template <typename T> __forceinline T DivideByMultiple(T value, size_t alignment)
	{
		return (T)((value + alignment - 1) / alignment);
	}

	template <typename T> __forceinline bool IsPowerOfTwo(T value)
	{
		return 0 == (value & (value - 1));
	}

	template <typename T> __forceinline bool IsDivisible(T value, T divisor)
	{
		return (value / divisor) * divisor == value;
	}

	__forceinline uint8_t Log2(uint64_t value)
	{
		unsigned long mssb; // most significant set bit
		unsigned long lssb; // least significant set bit

		// If perfect power of two (only one set bit), return index of bit.  Otherwise round up
		// fractional log by adding 1 to most signicant set bit's index.
		if (_BitScanReverse64(&mssb, value) > 0 && _BitScanForward64(&lssb, value) > 0)
			return uint8_t(mssb + (mssb == lssb ? 0 : 1));
		else
			return 0;
	}

	template <typename T> __forceinline T AlignPowerOfTwo(T value)
	{
		return value == 0 ? 0 : 1 << Log2(value);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////

	inline size_t HashCore(const uint32_t* const Begin, const uint32_t* const End, size_t Hash)
	{
#if ENABLE_SSE_CRC32
		const uint64_t* Iter64 = (const uint64_t*)HashInternal::AlignUp(Begin, 8);
		const uint64_t* const End64 = (const uint64_t* const)HashInternal::AlignDown(End, 8);

		// If not 64-bit aligned, start with a single u32
		if ((uint32_t*)Iter64 > Begin)
		{
			Hash = _mm_crc32_u32((uint32_t)Hash, *Begin);
		}

		// Iterate over consecutive u64 values
		while (Iter64 < End64)
		{

			Hash = _mm_crc32_u64((uint64_t)Hash, *Iter64++);
		}

		// If there is a 32-bit remainder, accumulate that
		if ((uint32_t*)Iter64 < End)
		{
			Hash = _mm_crc32_u32((uint32_t)Hash, *(uint32_t*)Iter64);
		}
#else
		// An inexpensive hash for CPUs lacking SSE4.2
		for (const uint32_t* Iter = Begin; Iter < End; ++Iter)
		{
			Hash = 16777619U * Hash ^ *Iter;
		}
#endif
		return Hash;
	}
} // namespace HashInternal

namespace Hash
{
	template <typename T> 
	inline size_t MakeHash(const T* StateDesc, size_t Count = 1, size_t Hash = 2166136261U)
	{
		static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
		return HashInternal::HashCore((uint32_t*)StateDesc, (uint32_t*)(StateDesc + Count), Hash);
	}
}