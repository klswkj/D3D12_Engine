#include "stdafx.h"
#include "CustomMemory.h"
#include "CPULinearAllocator.h"

namespace customMemory
{
	static std::mutex s_mutex;

	CPULinearAllocator s_CPULinearAllocator;

	void Initialize()
	{
#if ( 199711L <= __cplusplus )

#else
		ASSERT(false, "Configure error a compiler with support for c11 language features is required.");
#endif
	}

	void Destroy()
	{
		s_CPULinearAllocator.DestroyAll();
	}

	void* Allocate(size_t Size)
	{
		CPULinearBuffer ret = s_CPULinearAllocator.Allocate(Size);

		return ret.pData;
	}
}
/*
template<typename T>
ScopedPtr<T> MakeScoped(void)
{
	CPULinearBuffer ret = customMemory::s_CPULinearAllocator.Allocate(sizeof(T));
	T* p = new(ret.pData) T();
	ScopedPtr<T> ptr(p);

	return ptr;
}

template<typename T, typename ...Args>
ScopedPtr<T> MakeScopedArgs(Args&& ... args)
{
	CPULinearBuffer ret = customMemory::s_CPULinearAllocator.Allocate(size_t(T));
	ScopedPtr<T> ptr(new(ret.pData) T(std::forward<Args>(args)...));

	return ptr;
}
*/