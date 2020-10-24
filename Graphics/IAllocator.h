#pragma once
#include <windows.h>
#include <stdint.h>
#include <utility>
#include <rpcndr.h>
#include <assert.h>

constexpr uint32_t CMaxStackHeapSize = 1024 * 1024 * 64; //64MB
constexpr uint32_t CMaxScratchSize = 1024 * 1024 * 16; //16MB

enum MemorySpaceType
{
	kMemStackSpace,
	kMemScratchSpace,
	kMemFrameSpace
};

class IAllocator
{
public:
	virtual void* Alloc(size_t size) = 0;
	virtual void Free(byte* buffer) = 0;
	virtual void Reset() {};
	virtual ~IAllocator() {}
};

class SystemHeapAllocator : public IAllocator
{
public:
	static const SystemHeapAllocator& Get()
	{
		static SystemHeapAllocator allocator = SystemHeapAllocator();
		return allocator;
	}

	virtual void* Alloc(size_t size) override
	{
		return (void*)new byte[size];
	}
	virtual void Free(byte* buffer) override
	{
		delete[] buffer;
	}
};

class LinearAllocator : public IAllocator
{
public:
	static LinearAllocator* Instance;
	explicit LinearAllocator(size_t sizeInBytes, IAllocator* allocator = nullptr)
	{
		if (!allocator)
		{
			allocator = (IAllocator*)&SystemHeapAllocator::Get();
		}
		parent = allocator;
		Instance = this;
		buffer = (byte*)allocator->Alloc(sizeInBytes);
		current = buffer;
		totalSize = sizeInBytes;
		memset(buffer, 0, totalSize);
	}

	virtual void* Alloc(size_t size) override
	{
		assert((current + size) < (buffer + totalSize));
		byte* alloc = current;
		current += size;
		return alloc;
	}

	virtual void Free(byte* buff) override
	{
		current = buff;
	}

	virtual void Reset() override
	{
		current = buffer;
	}

	~LinearAllocator()
	{
		parent->Free(buffer);
	}

	static LinearAllocator* GetInstance()
	{
		return Instance;
	}

private:
	byte* buffer = nullptr;
	byte* current = nullptr;
	size_t		totalSize = 0;
	IAllocator* parent = nullptr;
};

class StackAllocator : public IAllocator
{
public:
	StackAllocator() {}

	void Initialize(size_t sizeInBytes, IAllocator* allocator = nullptr)
	{
		if (!allocator)
		{
			allocator = LinearAllocator::GetInstance();
		}
		parent = allocator;
		buffer = (byte*)allocator->Alloc(sizeInBytes);
		current = buffer;
		totalSize = sizeInBytes;
		marker = buffer;
	}

	virtual void* Alloc(size_t size) override
	{
		assert((current + size) <= (buffer + totalSize));
		byte* alloc = current;
		current += size;
		return alloc;
	}

	virtual void Free(byte* buff) override
	{
		current = buff;
	}

	byte* Push()
	{
		marker = current;
		return marker;
	}

	void Pop(byte* marker)
	{
		current = marker;
	}

	virtual void Reset() override
	{
		current = buffer;
	}

	~StackAllocator()
	{
		parent->Free(buffer);
	}

private:
	byte*       buffer = nullptr;
	byte*       current = nullptr;
	byte*       marker = nullptr;
	size_t		totalSize = 0;
	IAllocator* parent = nullptr;
};


namespace custom
{
	void* Alloc(size_t sizeInBytes)
	{
		return LinearAllocator::Instance->Alloc(sizeInBytes);
	}
	void Free(void* buffer)
	{
		LinearAllocator::Instance->Free((byte*)buffer);
	}

	static IAllocator* GetDefaultAllocator()
	{
		return LinearAllocator::GetInstance();
	}

	template<typename T, typename ...Args>
	T* Alloc(Args&& ... args)
	{
		auto alloc = Alloc(sizeof(T));
		return new(alloc) T(std::forward<Args>(args)...);
	}

	template<typename T>
	T* AllocArray(size_t count)
	{
		auto alloc = Alloc(sizeof(T) * count);
		return (T*)alloc;
	}

	template<typename T>
	void Free(T* buff)
	{
		buff->~T();
		Free((void*)buff);
	}
}

template<typename T>
class PooledPtr
{
public:
	explicit PooledPtr(T* p)
	{
		ptr = p;
	}

	PooledPtr(PooledPtr&& p) :
		ptr(p.ptr)
	{
		p.ptr = nullptr;
	}

	PooledPtr& operator=(const PooledPtr& rhs) = delete;

	PooledPtr& operator=(PooledPtr&& rhs)
	{
		ptr = rhs.ptr;
		rhs.ptr = nullptr;
		return *this;
	}

	PooledPtr() {}

	T& operator * () { return *ptr; }
	T* operator -> () { return ptr; }
	T* operator -> () const { return ptr; }
	T* Get() { return ptr; }
	T* Get() const { return ptr; }
	//For compatibility
	T* get() { return ptr; }
	T* get() const { return ptr; }

	~PooledPtr()
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
static PooledPtr<T> MakePooled()
{
	auto allocator = LinearAllocator::GetInstance();
	return MakePooled<T>(allocator);
}

template<typename T>
static PooledPtr<T> MakePooled(IAllocator* allocator)
{
	auto alloc = allocator->Alloc(sizeof(T));
	T* p = new(alloc) T();
	PooledPtr<T> ptr(p);
	return ptr;
}

static PooledPtr<const char> MakePooled(const char* text, size_t size)
{
	auto allocator = LinearAllocator::GetInstance();
	size = sizeof(char) * (size);
	auto p = allocator->Alloc(size);
	memcpy_s(p, size, text, size);

	char* p2 = new(p) char(text[0]);
	PooledPtr<const char> ptr(p2);
	return ptr;
}

template<typename T, typename ...Args>
static PooledPtr<T> MakePooledArgs(Args&& ... args)
{
	auto allocator = LinearAllocator::GetInstance();
	auto alloc = allocator->Alloc(sizeof(T));
	PooledPtr<T> ptr(new(alloc) T(std::forward<Args>(args)...));
	return ptr;
	//	return MakePooledArgsAlloc(std::forward<Args>(args)..., allocator);
}

template<typename T, typename ...Args>
static PooledPtr<T> MakePooledArgsAlloc(Args&&... args, IAllocator* allocator)
{
	auto alloc = allocator->Alloc(sizeof(T));
	PooledPtr<T> ptr(new(alloc) T(std::forward<Args>(args)...));
	return ptr;
}