#include "stdafx.h"
#include "D3D12SegListAllocator.h"
#include "CustomD3D12Adapter.h"
#include "CustomD3D12GPUNode.h"
#include "GPUResource.h"

D3D12SegHeap::D3D12SegHeap
(
	custom::D3D12GPUNode* const pGPUNode,
	const D3D12_NODE_MASK visibileNodeMask,
	ID3D12Heap* const NewHeap,
	D3D12SegList* const pOwner,
	const uint32_t arrayIndex
)
	:
	// ManagedD3D12Heap(Parent, VisibileNodeMask),
	ManagedD3D12Heap(),
	m_pOwnerList(pOwner),
	m_ArrayIndex(arrayIndex),
	m_FirstFreeOffset(0u),
	m_RefCount(0u)
{

	SetHeap(NewHeap);
	BeginTrackingResidency();
}

uint32_t D3D12SegList::AllocateBlock
(
	ID3D12Device* const      pParentDevice,
	const UINT               creationNodeMask,
	const UINT               visibleNodeMask,
	const ::D3D12_HEAP_TYPE  heapType,
	const ::D3D12_HEAP_FLAGS heapFlags,
	_Outptr_ D3D12SegHeap*& pOutHeap
)
{
	uint32_t Offset = 0u;

	{
		// custom::Scoped_CS Lock(m_CS);
		::EnterCriticalSection(&m_CS);

		if (m_pFreeSegHeaps.size())
		{
			const int32_t LastFreeHeapIndex = m_pFreeSegHeaps.size() - 1;
			pOutHeap = m_pFreeSegHeaps[LastFreeHeapIndex];
			Offset = pOutHeap->AllocateBlock(m_BlockSize);

			ASSERT(Offset <= m_HeapSize - m_BlockSize);

			if (pOutHeap->IsFull(m_HeapSize))
			{
				// Heap is full
				pOutHeap->m_ArrayIndex = -1;
				m_pFreeSegHeaps.erase(m_pFreeSegHeaps.end());
			}
		}
		else
		{
			pOutHeap = CreateBackingHeap(pParentDevice, creationNodeMask, visibleNodeMask, heapType, heapFlags);
			Offset = pOutHeap->AllocateBlock(m_BlockSize);
#if defined(_DEBUG)
			m_Debug_TotalBytesAllocated += m_HeapSize;
#endif
		}
		::LeaveCriticalSection(&m_CS);
	}

	return Offset;
}

D3D12SegHeap* D3D12SegList::CreateBackingHeap
(
	::ID3D12Device* const    pParentDevice,
	const UINT               creationNodeMask,
	const UINT               visibleNodeMask,
	const ::D3D12_HEAP_TYPE  heapType,
	const ::D3D12_HEAP_FLAGS heapFlags
)
{
	ASSERT(pParentDevice);
	// CS can be unlocked at this point and re-locked before adding it to FreeHeaps
	// but doing so may cause multiple heaps to be created
	ID3D12Heap* D3DHeap  = nullptr;

	D3D12_HEAP_DESC Desc = {};
	Desc.SizeInBytes = m_HeapSize;
	Desc.Properties  = CD3DX12_HEAP_PROPERTIES(heapType, creationNodeMask, visibleNodeMask);
	Desc.Flags       = heapFlags;

	ASSERT(pParentDevice->CreateHeap(&Desc, IID_PPV_ARGS(&D3DHeap)));

	D3D12SegHeap* Ret = new D3D12SegHeap(D3DHeap, this, m_pFreeSegHeaps.size());
	m_pFreeSegHeaps.push_back(Ret);
	return Ret;
}

#pragma region SEG_LIST_ALLOCATOR

D3D12SegListAllocator::D3D12SegListAllocator
(
	custom::D3D12GPUNode* pGPUNode,
	::D3D12_NODE_MASK  visibleNodeMask,
	::D3D12_HEAP_TYPE  heapType,
	::D3D12_HEAP_FLAGS heapFlags,
	uint32_t minPoolSize,
	uint32_t minNumToPool,
	uint32_t maxPoolSize
)
	: 
	m_pGPUNode(pGPUNode),
	m_VisibleNodeMask(visibleNodeMask),
	m_HeapType(heapType), 
	m_HeapFlags(heapFlags), 
	m_MinPoolSize(minPoolSize), 
	m_MinNumToPool(minNumToPool), 
	m_MaxPoolSize(maxPoolSize)
#if defined(_DEBUG)
	, m_Debug_TotalBytesRequested(0ull)
#endif
{
	ASSERT(pGPUNode);
}

void D3D12SegListAllocator::Deallocate
(
	custom::GPUResource* pPlacedResource,
	uint32_t             offset,
	uint32_t             sizeInBytes
)
{
	ASSERT(pPlacedResource);

	uint64_t FrameFenceValue = m_pGPUNode->GetParentAdapter()->GetFrameFenceValue();

	{
		// custom::Scoped_CS ScopedCS(m_DeferredDeletionCS);
		::EnterCriticalSection(&m_DeferredDeletionCS);

		ASSERT(m_FenceValues.size() == m_DeferredDeletionQueues.size());
		ASSERT(!m_FenceValues.size() || m_FenceValues.back() <= FrameFenceValue);

		int32_t LastIndex = m_FenceValues.size() - 1;
		if (LastIndex < 0 || m_FenceValues[LastIndex] != FrameFenceValue)
		{
			++LastIndex;
			m_FenceValues.push_back(FrameFenceValue);
			// m_DeferredDeletionQueues.AddDefaulted();
		}
		// new (m_DeferredDeletionQueues[LastIndex]) FRetiredBlock(pPlacedResource, offset, sizeInBytes);
		// m_DeferredDeletionQueues[LastIndex].push_back(D3D12SegListAllocator::RetiredBlock(pPlacedResource, offset, sizeInBytes));
		m_DeferredDeletionQueues[LastIndex].emplace_back(std::move(D3D12SegListAllocator::RetiredBlock(pPlacedResource, offset, sizeInBytes)));
		static_assert(false, "Must Test this phrase with compile.");

		::LeaveCriticalSection(&m_DeferredDeletionCS);
	}
}

void D3D12SegListAllocator::FreeRetiredBlocks(std::vector<std::vector<D3D12SegListAllocator::RetiredBlock>>& pendingDeleteRetiredBlocks)
{
	for (auto& RetiredBlocks : pendingDeleteRetiredBlocks)
	{
		for (auto& RetiredBlock : RetiredBlocks)
		{
			D3D12SegHeap* BackingHeap = static_cast<D3D12SegHeap*>(RetiredBlock.PlacedResource->GetPlacedManagedHeap());
			ASSERT(RetiredBlock.PlacedResource->GetReferenceCount() == 1);

			if (RetiredBlock.PlacedResource->GetReferenceCount() == 1)
			{
				delete RetiredBlock.PlacedResource;
				RetiredBlock.PlacedResource = nullptr;
			}
			else
			{
				// 일단 두번 ASSERT 이후 삭제할 것.
				ASSERT(false);
			}

			D3D12SegList* Owner = BackingHeap->m_pOwnerList;
			ASSERT(Owner);
			Owner->FreeBlock(BackingHeap, RetiredBlock.Offset);
			OnFree(RetiredBlock.Offset, BackingHeap, RetiredBlock.ResourceSize);
		}
	}
}

void D3D12SegListAllocator::CleanUpAllocations()
{
	std::vector<std::vector<RetiredBlock>> PendingDeletes;

	{
		custom::D3D12Adapter* pParentAdapter = m_pGPUNode->GetParentAdapter();
		uint64_t FrameFenceValue             = pParentAdapter->GetFrameFenceValue();

		uint32_t NumToRemove = 0u;
		custom::Scoped_CS ScopedCS(m_DeferredDeletionCS);
		
		size_t DeferredDeletionQueueSize = m_DeferredDeletionQueues.size();

		ASSERT(m_FenceValues.size() == DeferredDeletionQueueSize);

		for (int32_t i = 0; i < m_DeferredDeletionQueues.size(); ++i)
		{
			if (pParentAdapter->IsFrameFenceComplete(m_FenceValues[i]))
			{
				++NumToRemove;
				// PendingDeletes.Add(MoveTemp(m_DeferredDeletionQueues[i]));
				PendingDeletes.emplace_back(std::move(m_DeferredDeletionQueues[i]));
			}
			else
			{
				break;
			}
		}

		// Test
		ASSERT(DeferredDeletionQueueSize == m_DeferredDeletionQueues.size());

		if (NumToRemove)
		{
			// m_FenceValues.RemoveAt(0, NumToRemove);
			// m_DeferredDeletionQueues.RemoveAt(0, NumToRemove);

			m_FenceValues.erase(m_FenceValues.begin(), m_FenceValues.begin() + NumToRemove);
			m_FenceValues.shrink_to_fit();

			m_DeferredDeletionQueues.erase(m_DeferredDeletionQueues.begin(), m_DeferredDeletionQueues.begin() + NumToRemove);
			m_DeferredDeletionQueues.shrink_to_fit();
		}
	}

	FreeRetiredBlocks(PendingDeletes);
}

void D3D12SegListAllocator::Destroy()
{
	{
		custom::Scoped_CS ScopedCS(m_DeferredDeletionCS);

		ASSERT(m_FenceValues.size() == m_DeferredDeletionQueues.size());
		FreeRetiredBlocks(m_DeferredDeletionQueues);
		m_FenceValues.clear();
		m_FenceValues.shrink_to_fit();
		m_DeferredDeletionQueues.clear();
		m_DeferredDeletionQueues.shrink_to_fit();
#if defined(_DEBUG)
		VerifyEmpty();
#endif
	}
	{
		custom::Scoped_SRWLock ScopedSRW(m_SegListsSRWLock, custom::eSRWScopeLockType::eExclusive);

		for (auto& Pair : m_SegListsMap)
		{
			D3D12SegList*& SegList = Pair.second;
			ASSERT(SegList);
			delete SegList;
			SegList = nullptr;
		}

		m_SegListsMap.clear();
	}
}

#if defined(_DEBUG)
//{
	void D3D12SegListAllocator::VerifyEmpty()
	{
		custom::Scoped_CS ScopedCS(m_Debug_SegListTrackedAllocationCS);

		if (m_Debug_SegListTrackedAllocations.size() != 0)
		{
			for (auto& LeakTrack : m_Debug_SegListTrackedAllocations)
			{
				DumpStack(LeakTrack);
			}
		}

		ASSERT(m_Debug_TotalBytesRequested == 0);
	}

	// Following these menu.
	// Project Properties(In Visual Studio) - Configuration Property - Linker - Advanced - Profile -> Yes(/Profile)
	void D3D12SegListAllocator::DumpStack(const D3D12SegListAllocatorLeakTrack & LeakTrack)
	{
		::printf("Leaking Allocation Heap %p Offset %d\nStack Dump\n", LeakTrack.pHeap, LeakTrack.Offset);

		for (uint32_t i = 0; i < LeakTrack.StackDepth; ++i)
		{
			const size_t STRING_SIZE = 16 * 1024;
			unsigned char StackTrace[STRING_SIZE];
			StackTrace[0] = 0;
			// FPlatformStackWalk::ProgramCounterToHumanReadableString(i, LeakTrack.Stack[i], StackTrace, STRING_SIZE, 0);
			// UE_LOG(LogD3D12RHI, Warning, TEXT("%d %S\n"), i, StackTrace);
		}
	}

	void D3D12SegListAllocator::OnAlloc(uint32_t Offset, void* Heap, uint32_t Size)
	{
		m_Debug_TotalBytesRequested += Size;

		D3D12SegListAllocatorLeakTrack LeakTrack = {};
		LeakTrack.Offset = Offset;
		LeakTrack.pHeap  = Heap;
		LeakTrack.Size   = Size;
		LeakTrack.StackDepth = ::CaptureStackBackTrace(&LeakTrack.Stack[0], SEGLIST_ALLOC_TRACK_LEAK_STACK_DEPTH);


		static_assert(false, "Here.");
		/*
		WORD
	NTAPI
	RtlCaptureStackBackTrace(
		_In_ DWORD FramesToSkip,
		_In_ DWORD FramesToCapture,
		_Out_writes_to_(FramesToCapture,return) PVOID* BackTrace,
		_Out_opt_ PDWORD BackTraceHash
		);
		*/
		{
			// custom::Scoped_CS ScopedCS(m_Debug_SegListTrackedAllocationCS);
			::EnterCriticalSection(&m_Debug_SegListTrackedAllocationCS);
			ASSERT(m_Debug_SegListTrackedAllocations.find(LeakTrack) == m_Debug_SegListTrackedAllocations.end());
			m_Debug_SegListTrackedAllocations.insert(LeakTrack);
			::LeaveCriticalSection(&m_Debug_SegListTrackedAllocationCS);
		}
	}

	void D3D12SegListAllocator::OnFree(uint32_t Offset, void* Heap, uint32_t Size)
	{
		m_Debug_TotalBytesRequested -= Size;

		D3D12SegListAllocatorLeakTrack LeakTrack;
		LeakTrack.Offset = Offset;
		LeakTrack.pHeap = Heap;

		{
			// custom::Scoped_CS ScopedCS(m_Debug_SegListTrackedAllocationCS);
			::EnterCriticalSection(&m_Debug_SegListTrackedAllocationCS);
			std::set<D3D12SegListAllocatorLeakTrack>::iterator LeakTrackIter = m_Debug_SegListTrackedAllocations.find(LeakTrack);

			ASSERT(LeakTrackIter != m_Debug_SegListTrackedAllocations.end()); // element being freed was not found.

			if (LeakTrackIter->Size != Size)
			{
				::printf("Mismatched alloc/free size %d != %d, %p/%08x", LeakTrackIter->Size, Size, LeakTrackIter->pHeap, LeakTrackIter->Offset);
				DumpStack(*LeakTrackIter);
				ASSERT(false); //element being freed had incorrect size. 
			}

			ASSERT(LeakTrackIter != m_Debug_SegListTrackedAllocations.end());
			m_Debug_SegListTrackedAllocations.erase(LeakTrackIter);
			ASSERT(m_Debug_SegListTrackedAllocations.find(LeakTrack) == m_Debug_SegListTrackedAllocations.end());
			::LeaveCriticalSection(&m_Debug_SegListTrackedAllocationCS);
		}
	}
//}
#endif // end _debug
#pragma endregion SEG_LIST_ALLOCATOR