#pragma once
#include "CPULinearAllocator.h"

// TODO : 1. Deallocate on ScopedPtr with returing CPULinearBuffer.
// TODO : 2. Make Aligned memory buffers are to be controlled.

// Usage -> ScopedPtr<Someting> pSomething = MakedScoped<Someting>();
//       -> ScopedPtr<Someting> pSomething = MakeScopedArgs<Someting>(Arg1, Arg2, ...);

namespace customMemory
{
	void Initialize();
	void Destroy();

	void* Allocate(size_t Size);
}

template<typename T>
class ScopedPtr
{
public:
	ScopedPtr() = default;

	explicit ScopedPtr(T* p)
	{
		ptr = p;
	}

	ScopedPtr(ScopedPtr&& p)
		: ptr(p.ptr)
	{
		p.ptr = nullptr;
	}

	ScopedPtr& operator=(const ScopedPtr& rhs) = delete;

	ScopedPtr& operator=(ScopedPtr&& rhs)
	{
		ptr = rhs.ptr;
		rhs.ptr = nullptr;
		return *this;
	}

	T& operator *()
	{
		return *ptr;
	}
	T* operator ->()
	{
		return ptr;
	}
	T* operator ->() const
	{
		return ptr;
	}
	T* Get()
	{
		return ptr;
	}
	T* Get() const
	{
		return ptr;
	}

	// For compatibility, But not used.
	T* get() { return ptr; }
	T* get() const { return ptr; }

	~ScopedPtr()
	{
		if (ptr)
		{
			ptr->~T();
			ptr = nullptr;
		}
	}
private:
	T* ptr = nullptr;
};

template<typename T>
inline ScopedPtr<T> MakeScoped(void)
{
	T* p = new(customMemory::Allocate(sizeof(T))) T();

	ScopedPtr<T> ptr(p);

	return ptr;
}

template<typename T, typename ...Args>
inline ScopedPtr<T> MakeScopedArgs(Args&& ... args)
{
	ScopedPtr<T> ptr(new(customMemory::Allocate(sizeof(T))) T(std::forward<Args>(args)...));

	return ptr;
}

/*
#pragma once
#include "CPULinearAllocation.h"

namespace customMemory
{
	void Initialize();
	void Destroy();

	template<typename T>
	class ScopedPtr
	{
	public:
		ScopedPtr() = default;

		explicit ScopedPtr(T* p)
		{
			ptr = p;
		}

		ScopedPtr(ScopedPtr&& p)
			: ptr(p.ptr)
		{
			p.ptr = nullptr;
		}

		ScopedPtr& operator=(const ScopedPtr& rhs) = delete;

		ScopedPtr& operator=(ScopedPtr&& rhs)
		{
			ptr = rhs.ptr;
			rhs.ptr = nullptr;
			return *this;
		}

		T& operator *()
		{
			return *ptr;
		}
		T* operator ->()
		{
			return ptr;
		}
		T* operator ->() const
		{
			return ptr;
		}
		T* Get()
		{
			return ptr;
		}
		T* Get() const
		{
			return ptr;
		}

		// For compatibility, But not used.
		T* get() { return ptr; }
		T* get() const { return ptr; }

		~ScopedPtr()
		{
			if (ptr)
			{
				ptr->~T();
				ptr = nullptr;
			}
		}
	private:
		T* ptr = nullptr;
	};

	template<typename T>
	ScopedPtr<T> MakeScoped(void);

	template<typename T, typename ...Args>
	ScopedPtr<T> MakeScoped(Args&& ... args);
}

*/

/*
#pragma once
#include "CPULinearAllocation.h"

namespace customMemory
{
	static CPULinearAllocator s_CPULinearAllocator;

	inline void Initialize()
	{
#if ( 199711L <= __cplusplus )

#else
		ASSERT(false);
#endif
	}
	inline void Destroy()
	{
		s_CPULinearAllocator.DestroyAll();
	}

	template<typename T>
	class ScopedPtr
	{
	public:
		ScopedPtr() = default;

		explicit ScopedPtr(T* p)
		{
			ptr = p;
		}

		ScopedPtr(ScopedPtr&& p)
			: ptr(p.ptr)
		{
			p.ptr = nullptr;
		}

		ScopedPtr& operator=(const ScopedPtr& rhs) = delete;

		ScopedPtr& operator=(ScopedPtr&& rhs)
		{
			ptr = rhs.ptr;
			rhs.ptr = nullptr;
			return *this;
		}

		T& operator *()
		{
			return *ptr;
		}
		T* operator ->()
		{
			return ptr;
		}
		T* operator ->() const
		{
			return ptr;
		}
		T* Get()
		{
			return ptr;
		}
		T* Get() const
		{
			return ptr;
		}

		// For compatibility, But not used.
		T* get() { return ptr; }
		T* get() const { return ptr; }

		~ScopedPtr()
		{
			if (ptr)
			{
				ptr->~T();
				ptr = nullptr;
			}
		}
	private:
		T* ptr = nullptr;
	};

	template<typename T>
	inline ScopedPtr<T> MakeScoped(void)
	{
		CPULinearBuffer ret = s_CPULinearAllocator.Allocate(size_t(T));
		T* p = new(ret.pData) T();
		ScopedPtr<T> ptr(p);

		return ptr;
	}

	template<typename T, typename ...Args>
	inline ScopedPtr<T> MakeScoped(Args&& ... args)
	{
		CPULinearBuffer ret = s_CPULinearAllocator.Allocate(size_t(T));
		ScopedPtr<T> ptr(new(ret.pData) T(std::forward<Args>(args)...));

		return ptr;
	}
}
*/