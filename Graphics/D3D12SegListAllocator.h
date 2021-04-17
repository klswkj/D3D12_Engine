#pragma once
#include "ManagedD3D12Heap.h"
#include "CustomCriticalSection.h"

namespace custom
{
	class GPUResource;
	class D3D12GPUNode;
}

#if defined(_DEBUG)
#define SEGLIST_ALLOC_TRACK_LEAK_STACK_DEPTH 12
#endif

class D3D12SegHeap : public custom::ManagedD3D12Heap
{
private:
	friend class D3D12SegList;
	friend class D3D12SegListAllocator;

	D3D12SegHeap
	(
		custom::D3D12GPUNode* const pGPUNode,
		const D3D12_NODE_MASK visibileNodeMask,
		ID3D12Heap* const NewHeap,
		D3D12SegList* const pOwner,
		const uint32_t arrayIndex
	);

	virtual ~D3D12SegHeap() = default;

	D3D12SegHeap(const D3D12SegHeap&)            = delete;
	D3D12SegHeap(D3D12SegHeap&&)                 = delete;
	D3D12SegHeap& operator=(const D3D12SegHeap&) = delete;
	D3D12SegHeap& operator=(D3D12SegHeap&&)      = delete;

	bool bArrayIndexValid() const { return 0 <= m_ArrayIndex; }

	bool IsFull(const uint32_t heapSize) const
	{
		ASSERT(m_FirstFreeOffset <= heapSize);
		return !m_FreeBlockOffsets.size() && (m_FirstFreeOffset == heapSize);
	}

	bool IsEmpty(const uint32_t blockSize) const
	{
		return (m_FreeBlockOffsets.size() * blockSize == m_FirstFreeOffset);
	}

	// @return - In-heap offset of the allocated block
	uint32_t AllocateBlock(const uint32_t blockSize)
	{
		if (!m_FreeBlockOffsets.size())
		{
			uint32_t Ret = m_FirstFreeOffset;
			m_FirstFreeOffset += blockSize;
			return Ret;
		}
		else
		{
			uint32_t LastElement = m_FreeBlockOffsets.back();
			m_FreeBlockOffsets.erase(m_FreeBlockOffsets.end());

			return LastElement;
		}
	}

	inline uint32_t AddReference()
	{
		uint32_t IncreasedRef = ::InterlockedIncrement(&m_RefCount);
		ASSERT(IncreasedRef);
		return IncreasedRef;
	}
	inline uint32_t DecrementReference()
	{
		uint32_t DecreasedRef = ::InterlockedDecrement(&m_RefCount);

		ASSERT(DecreasedRef != -1);
		return DecreasedRef;
	}
	inline uint32_t GetReferenceCount()
	{
		return ::InterlockedGetValue(&m_RefCount);
	}

	std::vector<uint32_t> m_FreeBlockOffsets;
	D3D12SegList*         m_pOwnerList;
	uint32_t              m_ArrayIndex;
	uint32_t              m_FirstFreeOffset;
	volatile uint32_t     m_RefCount;
};

class D3D12SegList
{
private:
	friend class D3D12SegListAllocator;

	explicit D3D12SegList(uint32_t InBlockSize, uint32_t InHeapSize)
		
		: 
		m_BlockSize(InBlockSize), 
		m_HeapSize(InHeapSize),
		m_CS({})
#if defined(_DEBUG)
		, m_Debug_TotalBytesAllocated(0)
#endif
	{
		ASSERT(!(m_HeapSize % m_BlockSize) && (m_BlockSize < m_HeapSize));

		if (!m_CS.DebugInfo)
		{
			::InitializeCriticalSection(&m_CS);
		}
	}

	~D3D12SegList()
	{
#if defined(_DEBUG)
		// FScopeLock Lock(&m_CS);
		::EnterCriticalSection(&m_CS);

		ASSERT(m_BlockSize && m_HeapSize);

		for (const auto& FreeSegHeap : m_pFreeSegHeaps)
		{
			ASSERT(FreeSegHeap->GetReferenceCount() == 1);
		}

		::LeaveCriticalSection(&m_CS);

		::DeleteCriticalSection(&m_CS);
#endif
	}

	D3D12SegList(const D3D12SegList&)            = delete;
	D3D12SegList(D3D12SegList&&)                 = delete;
	D3D12SegList& operator=(const D3D12SegList&) = delete;
	D3D12SegList& operator=(D3D12SegList&&)      = delete;

	// @return - In-heap offset of the allocated block
	uint32_t AllocateBlock
	(
		ID3D12Device* const      pParentDevice,
		const UINT               creationNodeMask,
		const UINT               visibleNodeMask,
		const ::D3D12_HEAP_TYPE  heapType,
		const ::D3D12_HEAP_FLAGS heapFlags,
		_Outptr_ D3D12SegHeap*& pOutHeap
	);

	// Deferred deletion is handled by FD3D12SegListAllocator
	void FreeBlock(D3D12SegHeap* __restrict pSegHeap, uint32_t Offset)
	{
		custom::Scoped_CS Lock(m_CS);

		ASSERT(!(Offset % m_BlockSize) &&
			(Offset <= m_HeapSize - m_BlockSize) &&
			(this == pSegHeap->m_pOwnerList));

		const bool bFull = pSegHeap->IsFull(m_HeapSize);
		pSegHeap->m_FreeBlockOffsets.push_back(Offset);

		if (bFull)
		{
			// Heap was full
			ASSERT(!pSegHeap->bArrayIndexValid());
			m_pFreeSegHeaps.push_back(pSegHeap);
			pSegHeap->m_ArrayIndex = (uint32_t)(m_pFreeSegHeaps.size() - 1);
		}
		else if (pSegHeap->IsEmpty(m_BlockSize))
		{
			// Heap is empty
			// ASSERT(pSegHeap->GetRefCount() == 1);
			ASSERT(pSegHeap->bArrayIndexValid());
			// ASSERT(pSegHeap->m_ArrayIndex < m_pFreeSegHeaps.Num());
			const uint32_t SegHeapIndex  = pSegHeap->m_ArrayIndex;
			const uint32_t FreeHeapIndex = (uint32_t)(m_pFreeSegHeaps.size() - 1);

			m_pFreeSegHeaps.erase(m_pFreeSegHeaps.begin() + SegHeapIndex);

			if (SegHeapIndex != FreeHeapIndex)
			{
				m_pFreeSegHeaps[SegHeapIndex]->m_ArrayIndex = SegHeapIndex;
			}

#if defined(_DEBUG)
			m_Debug_TotalBytesAllocated -= m_HeapSize;
#endif
		}
	}

	D3D12SegHeap* CreateBackingHeap
	(
		::ID3D12Device* const    pParentDevice,
		const UINT               creationNodeMask,
		const UINT               visibleNodeMask,
		const ::D3D12_HEAP_TYPE  heapType,
		const ::D3D12_HEAP_FLAGS heapFlags
	);

	std::vector<D3D12SegHeap*> m_pFreeSegHeaps; // 32
	::CRITICAL_SECTION         m_CS; // 40
	uint32_t                   m_BlockSize; // 4
	uint32_t                   m_HeapSize; // 4

#if defined(_DEBUG)
public:
	uint64_t m_Debug_TotalBytesAllocated; // 8
#endif
};

Sizer<sizeof(D3D12SegList)> SizerTemp;                // 88 -> 64?? // To make reduce by 24
Sizer<sizeof(std::vector<D3D12SegHeap*>)> SizerTemp0; // 32
Sizer<sizeof(CRITICAL_SECTION)> SizerTemp1;           // 40
Sizer<sizeof(uint64_t)> SizerTemp2;                   // 08
Sizer<sizeof(D3D12SegHeap)> SizerD3D12SegHeap;        // 152
Sizer<sizeof(::SRWLOCK)> SizerSRWLOCK;                // 8

#if defined(_DEBUG)
// static_assert((sizeof(D3D12SegList) - sizeof(D3D12SegList::m_Debug_TotalBytesAllocated)) <= 64);
#endif

// For Texture Allocator
class D3D12SegListAllocator//  : public FD3D12DeviceChild, public FD3D12MultiNodeGPUObject
{
public:
	static constexpr uint32_t sm_kInvalidOffset = 0xffffffff;

	explicit D3D12SegListAllocator
	(
		custom::D3D12GPUNode* pGPUNode,
		::D3D12_NODE_MASK  visibleNodeMask,
		::D3D12_HEAP_TYPE  heapType,
		::D3D12_HEAP_FLAGS heapFlags,
		uint32_t minPoolSize,
		uint32_t minNumToPool,
		uint32_t maxPoolSize
	);

	~D3D12SegListAllocator()
	{
		ASSERT(!m_SegListsMap.size());
		ASSERT(!m_FenceValues.size());
		ASSERT(!m_DeferredDeletionQueues.size());
		VerifyEmpty();
	}

	D3D12SegListAllocator(const D3D12SegListAllocator&)            = delete;
	D3D12SegListAllocator(D3D12SegListAllocator&&)                 = delete;
	D3D12SegListAllocator& operator=(const D3D12SegListAllocator&) = delete;
	D3D12SegListAllocator& operator=(D3D12SegListAllocator&&)      = delete;

	uint32_t Allocate(uint32_t sizeInBytes, uint32_t alignment, _Outptr_ D3D12SegHeap*& pOutHeap)
	{
		ASSERT(!(alignment & alignment - 1));

		const uint32_t BlockSize = CalculateBlockSize(sizeInBytes, alignment);

		if (ShouldPool(BlockSize))
		{
			D3D12SegList* SegList = nullptr;

			{
				custom::Scoped_SRWLock ScopedSRW(m_SegListsSRWLock, custom::eSRWScopeLockType::eShared);

				std::map<uint32_t, D3D12SegList*>::const_iterator ppSegListPtr = m_SegListsMap.find(BlockSize);
				SegList = (ppSegListPtr == m_SegListsMap.end())? nullptr : (*ppSegListPtr).second;
			}

			if (!SegList)
			{
				const uint32_t HeapSize = CalculateHeapSize(BlockSize);

				{
					custom::Scoped_SRWLock ScopedSRW(m_SegListsSRWLock, custom::eSRWScopeLockType::eExclusive);

					std::map<uint32_t, D3D12SegList*>::const_iterator ppSegListPtr = m_SegListsMap.find(BlockSize);
					SegList = (ppSegListPtr == m_SegListsMap.end()) ?
						(*ppSegListPtr).second : new D3D12SegList(BlockSize, HeapSize);

					if (ppSegListPtr == m_SegListsMap.end())
					{
						SegList = new D3D12SegList(BlockSize, HeapSize);
						m_SegListsMap.emplace(BlockSize, SegList);
					}
					else
					{
						SegList = (*ppSegListPtr).second;
					}
				}
			}

			ASSERT(SegList);

			uint32_t ReturnOffset = SegList->AllocateBlock
			(
				m_pGPUNode->GetParentAdapter()->GetRawDevice(),
				m_pGPUNode->GetGPUNodeIndexMask().NodeMask,
				m_VisibleNodeMask.NodeMask,
				m_HeapType,
				m_HeapFlags,
				pOutHeap
			);

			ASSERT(ReturnOffset != sm_kInvalidOffset);

#if defined(_DEBUG)
			OnAlloc(ReturnOffset, (void*)pOutHeap, sizeInBytes);
#endif
			return ReturnOffset;
		}
		pOutHeap = nullptr;
		return sm_kInvalidOffset;
	}

	// void Deallocate(ID3D12Resource* pPlacedResource, uint32_t Offset, uint32_t SizeInBytes);
	void Deallocate(custom::GPUResource* pPlacedResource, uint32_t offset, uint32_t sizeInBytes);
	void CleanUpAllocations();
	void Destroy();

#if defined(_DEBUG)
	bool GetMemoryStats(_Inout_ uint64_t& totalAllocated_Out, _Inout_ uint64_t& totalUnused_Out)
	{
		custom::Scoped_CS ScopedCS(m_DeferredDeletionCS);
		custom::Scoped_SRWLock ScopedSRW(m_SegListsSRWLock, custom::eSRWScopeLockType::eExclusive);

		totalAllocated_Out = 0;

		for (const auto& Pair : m_SegListsMap)
		{
			const D3D12SegList* SegList = Pair.second;
			totalAllocated_Out += SegList->m_Debug_TotalBytesAllocated;
		}

		totalUnused_Out = totalAllocated_Out - ::InterlockedGetValue(&m_Debug_TotalBytesRequested);
		return true;
	}
#endif

private:
	static constexpr uint32_t CalculateBlockSize(uint32_t sizeInBytes, uint32_t alignment)
	{
		return (sizeInBytes + alignment - 1) & ~(alignment - 1);
	}

private:
	struct RetiredBlock
	{
		// Should Resource knows which heap it is from...
		custom::GPUResource* PlacedResource;
		uint32_t Offset;
		uint32_t ResourceSize;

		RetiredBlock
		(
			custom::GPUResource* InResource,
			uint32_t InOffset,
			uint32_t InResourceSize
		) 
			:
			PlacedResource(InResource),
			Offset(InOffset),
			ResourceSize(InResourceSize)
		{
		}
	};

	inline bool ShouldPool(uint32_t blockSize) const
	{
		return blockSize * 2 <= m_MaxPoolSize;
	}

	uint32_t CalculateHeapSize(uint32_t blockSize) const
	{
		ASSERT(m_MinPoolSize < m_MinPoolSize + blockSize - 1);
		uint32_t NumPooled = (m_MinPoolSize + blockSize - 1) / blockSize;

		if (NumPooled < m_MinNumToPool)
		{
			NumPooled = m_MinNumToPool;
		}

		const uint32_t MaxNumPooled = m_MaxPoolSize / blockSize;

		if (MaxNumPooled < NumPooled)
		{
			NumPooled = MaxNumPooled;
		}

		ASSERT((1 < NumPooled) &&
			(m_MinPoolSize <= NumPooled * blockSize) &&
			(NumPooled * blockSize <= m_MaxPoolSize));

		return (NumPooled * blockSize);
	}

	void FreeRetiredBlocks(std::vector<std::vector<D3D12SegListAllocator::RetiredBlock>>& pendingDeleteRetiredBlocks);

private:
	custom::D3D12GPUNode*                   m_pGPUNode;
	::D3D12_NODE_MASK                       m_VisibleNodeMask;
	std::map<const uint32_t, D3D12SegList*> m_SegListsMap;
	std::vector<uint64_t>                   m_FenceValues;
	std::vector<std::vector<D3D12SegListAllocator::RetiredBlock>>  m_DeferredDeletionQueues;
	
	mutable ::SRWLOCK          m_SegListsSRWLock;
	mutable ::CRITICAL_SECTION m_DeferredDeletionCS;

	const ::D3D12_HEAP_TYPE  m_HeapType;
	const ::D3D12_HEAP_FLAGS m_HeapFlags;
	const uint32_t m_MinPoolSize;
	const uint32_t m_MinNumToPool;
	const uint32_t m_MaxPoolSize;

#if defined(_DEBUG)
public:
	struct D3D12SegListAllocatorLeakTrack
	{
		uint32_t Offset;
		void* pHeap;
		uint32_t Size;
		uint32_t StackDepth;
		uint64_t Stack[SEGLIST_ALLOC_TRACK_LEAK_STACK_DEPTH]; // 12
		bool operator==(const D3D12SegListAllocatorLeakTrack& Other) const
		{
			return Offset == Other.Offset && pHeap == Other.pHeap;
		}
	};

	volatile uint64_t                        m_Debug_TotalBytesRequested;
	::CRITICAL_SECTION                       m_Debug_SegListTrackedAllocationCS;
	std::set<D3D12SegListAllocatorLeakTrack> m_Debug_SegListTrackedAllocations;

	void DumpStack(const D3D12SegListAllocatorLeakTrack& LeakTrack);
	void OnAlloc(uint32_t Offset, void* pHeap, uint32_t Size);
	void OnFree(uint32_t Offset, void* pHeap, uint32_t Size);
	void VerifyEmpty();
#endif
};