#include "stdafx.h"
#include "D3D12ResourceAllocator.h"
#include "D3D12ResourceLocation.h"
#include "CustomD3D12Adapter.h"
#include "CustomD3D12GPUNode.h"
#include "ManagedD3D12Heap.h"

#pragma region D3D12_BUDDY_ALLOCATOR

D3D12BuddyAllocator::D3D12BuddyAllocator
(
	// ID3D12Device* const        pDevice,
	custom::D3D12GPUNode* const pOwnedGPUNode,
	const D3D12_NODE_MASK creationMask,
	const D3D12_NODE_MASK visibleNodesMask,
	const eAllocationStaretgy eAllocationStrategy,
	const D3D12_HEAP_TYPE heapType,
	const D3D12_HEAP_FLAGS heapFlags,
	const D3D12_RESOURCE_FLAGS resourceFlags,
	const eAllocatorType eAllocatorType,
	const uint64_t maxSizeForPooling,
	const uint64_t maxBlockSize,
	const uint64_t minBlockSize //= _KB(64)
)
	:
	// BaseD3D12ResourceAllocator(pDevice, creationMask, visibleNodesMask, szName, heapType, resourceFlags, maxSizeForPooling),
	BaseD3D12ResourceAllocator(pOwnedGPUNode, creationMask, visibleNodesMask, heapType, resourceFlags, maxSizeForPooling),
	m_eAllocationStrategy(eAllocationStrategy),
	m_eAllocatorType(eAllocatorType),
	m_HeapFlags(heapFlags),
	m_MaxBlockSize(maxBlockSize),
	m_MinBlockSize(minBlockSize)
#if defined(_DEBUG)
	, m_Debug_SpaceUsed(0u),
	m_Debug_InternalFragmentation(0u),
	m_Debug_NumBlocksInDeferredDeletionQueue(0u),
	m_Debug_PeakUsage(0u),
	m_Debug_FailedAllocationSpace(0u)
#endif
{
	uint64_t MaxDivideMin = (m_MaxBlockSize / m_MinBlockSize);

	ASSERT((MaxDivideMin * m_MinBlockSize == m_MaxBlockSize) && // Evenly dividable  
		((MaxDivideMin & (MaxDivideMin - 1ull)) == 0ull));            // Power of two  

	m_MaxOrder = unitSizeToOrder(sizeToUnitSize(m_MaxBlockSize));

	ResetBuddyAllocator();
}

void D3D12BuddyAllocator::InitializeBuddyAllocator()
{
	ASSERT((m_pBackingHeap == nullptr) && (m_pBackingResource == nullptr));

	if (m_eAllocationStrategy == eAllocationStaretgy::ePlacedResource)
	{
		D3D12_HEAP_PROPERTIES HeapProps = {};
		HeapProps.Type                  = m_kHeapType;
		HeapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask      = m_GPUCreationMask.NodeMask;
		HeapProps.VisibleNodeMask       = m_GPUVisibilityMask.NodeMask;

		D3D12_HEAP_DESC Desc = {};
		Desc.SizeInBytes     = m_MaxBlockSize;
		Desc.Properties      = HeapProps;
		Desc.Alignment       = 0ULL;
		Desc.Flags           = m_HeapFlags;

		ID3D12Heap* NewBackingHeap = nullptr;

		// ASSERT(m_pGlobalDevice->CreateHeap(&Desc, IID_PPV_ARGS(&NewBackingHeap)));
		ASSERT_HR(m_pOwnedGPUNode->GetParentAdapter()->GetRawDevice()->CreateHeap(&Desc, IID_PPV_ARGS(&NewBackingHeap)));

#if defined(_DEBUG)
		static uint64_t s_NumBuddyAllocatorBackingHeaps = -1;

		wchar_t BackingHeapName[32];
		swprintf(BackingHeapName, _countof(BackingHeapName), L"BuddyAllocator Backing Heap %zu", ++s_NumBuddyAllocatorBackingHeaps);
		NewBackingHeap->SetName(BackingHeapName);
#endif
		// m_pBackingHeap = new FD3D12Heap(GetParentDevice(), GetVisibilityMask());
		// m_pBackingHeap->SetHeap(Heap);
		// m_pBackingHeap = NewBackingHeap;
		// 
		// // 여기 쪽 문제 해결해야할 듯, 일단 맨위에 ASSERT로 막아놓고
		// if (m_pBackingHeap != nullptr)
		// {
		//	delete m_pBackingHeap;
		//	m_pBackingHeap = nullptr;
		// }

		m_pBackingHeap = new custom::ManagedD3D12Heap(NewBackingHeap, m_pOwnedGPUNode->GetResidencyManager());
		
		// Only track resources that cannot be accessed on the CPU.
		if (D3D12Util::IsCPUInaccessible(m_kHeapType))
		{
			m_pBackingHeap->BeginTrackingResidency();
		}

		return;
	}
	else if (m_eAllocationStrategy == eAllocationStaretgy::eCommittedResource)
	{
		// VERIFYD3D12RESULT(Adapter->CreateBuffer(m_kHeapType, GetGPUMask(), GetVisibilityMask(), m_MaxBlockSize, m_pBackingResource.GetInitReference(), TEXT("Resource Allocator Underlying Buffer"), m_ResourceFlags));
		D3D12_HEAP_PROPERTIES HeapProps = {};
		HeapProps.Type                  = m_kHeapType;
		HeapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask      = m_GPUCreationMask.NodeMask;
		HeapProps.VisibleNodeMask       = m_GPUVisibilityMask.NodeMask;
		
		// D3D12_HEAP_DESC HeapDesc = {};
		// HeapDesc.SizeInBytes = m_MaxBlockSize;
		// HeapDesc.Properties = HeapProps;
		// HeapDesc.Alignment = 0;
		// HeapDesc.Flags = m_HeapFlags;

		D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_MaxBlockSize, m_ResourceFlags);
		ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
		ResourceDesc.Alignment          = 0U;
		ResourceDesc.Width              = m_MaxBlockSize;
		ResourceDesc.Height             = 1U;
		ResourceDesc.DepthOrArraySize   = (UINT16)1;
		ResourceDesc.MipLevels          = (UINT16)1;
		ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
		ResourceDesc.SampleDesc.Count   = 1U;
		ResourceDesc.SampleDesc.Quality = 0U;
		ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ResourceDesc.Flags              = m_ResourceFlags;

		ID3D12Resource* pNewBackingResource = nullptr;

		// ASSERT_HR(m_pGlobalDevice->CreateCommittedResource(&HeapProps, m_HeapFlags, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&pNewBackingResource)));
		ASSERT_HR(m_pOwnedGPUNode->GetParentAdapter()->GetRawDevice()->CreateCommittedResource(&HeapProps, m_HeapFlags, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&pNewBackingResource)));

#if defined(_DEBUG)
		static uint64_t s_NumBuddyAllocatorBackingResources = -1;

		wchar_t BackingResourceName[32];
		swprintf(BackingResourceName, _countof(BackingResourceName), L"BuddyAllocator Backing Heap %zu", ++s_NumBuddyAllocatorBackingResources);
		pNewBackingResource->SetName(BackingResourceName);
#endif
		// 여기 쪽 문제 해결해야할 듯, 일단 맨위에 ASSERT로 막아놓고
		// if (m_pBackingResource != nullptr)
		// {
		// 	delete m_pBackingResource;
		// 	m_pBackingResource = nullptr;
		// }

		if (D3D12Util::IsCPUWritable(m_kHeapType))
		{
			m_pBackingResource->ResourceMap();
		}

		return;
	}

	ASSERT(false);
}
void D3D12BuddyAllocator::CleanUpAllocations(uint64_t fenceValue)
{
	custom::Scoped_CS Lock(m_RAII_CS);

	// 근데 여기서 문제가, RetiredBlock에 기록된 FrameFenceValue가 끝났어도,
	// 기록된 후에 다른 Queue에서 똑같은 Resource를 사용해서 
	// 증가된 FrameFenceValue를 기록하고 다른 RetiredBlock을 만들어서 계속 쓰고 있으면?
	// => Reference변수 추가해서 해결.

	uint32_t PopCount = 0;

	for (size_t i = 0; i < m_DeferredDeletionPlacedResourceQueue.size(); ++i)
	{
		RetiredBlock& Block = m_DeferredDeletionPlacedResourceQueue[i];

		if (fenceValue < Block.FenceValue)
		{
			deallocateInternalBuddyAllocator(Block);
			PopCount = (uint32_t)(i + 1);
#if defined(_DEBUG)
			++m_Debug_NumBlocksInDeferredDeletionQueue;
#endif
		}
		else
		{
			break;
		}
	}

	// I wonder
	if (PopCount)
	{
		::ZeroMemory(m_DeferredDeletionPlacedResourceQueue.data(), sizeof(D3D12BuddyAllocator::RetiredBlock) * PopCount);
	}
}

uint64_t D3D12BuddyAllocator::GetAllocationOffsetInBytes(const custom::D3D12BuddyAllocatorPrivateData& allocatorData) const
{
	return uint64_t(allocatorData.Offset * m_MinBlockSize);
}

void D3D12BuddyAllocator::ResetBuddyAllocator()
{
	// Clear the free blocks collection
	m_FreeBlocks.clear();

	// Initialize the pool with a free inner block of max inner block size
	m_FreeBlocks.resize((size_t)m_MaxOrder + 1ul);
	m_FreeBlocks[m_MaxOrder].insert(0u);
}

uint32_t D3D12BuddyAllocator::allocateBlockRecursiveBuddyAllocator(uint32_t order)
{
	uint32_t offset = 0;

	ASSERT(m_MaxOrder < order); // Can't allocate a block that large  

	if (m_FreeBlocks[order].size() == 0)
	{
		// No free nodes in the requested pool.  
		// Try to find a higher-order block and split it.  
		uint32_t left = allocateBlockRecursiveBuddyAllocator(order + 1);

		uint32_t size = orderToUnitSize(order);

		uint32_t right = left + size;

		m_FreeBlocks[order].insert(right); // Add the right block to the free pool  

		offset = left; // Return the left block  
	}
	else
	{
		// TSet<uint32>::TConstIterator it(FreeBlocks[order]);
		// offset = *it;
		// m_FreeBlocks[order].remove(*it);

		std::set<uint32_t>::const_iterator iter = m_FreeBlocks[order].begin();
		m_FreeBlocks[order].erase(*iter);
	}

	return offset;
}
void D3D12BuddyAllocator::allocateBuddyAllocator(const uint64_t sizeInBytes, const uint32_t alignment, custom::D3D12ResourceLocation& resourceLocation)
{
	// FScopeLock Lock(&m_CS);
	custom::Scoped_CS ScopedCS(m_RAII_CS);

	uint64_t SizeToAllocate = sizeInBytes;

	// If the alignment doesn't match the block size
	if (alignment != 0 && m_MinBlockSize % alignment != 0)
	{
		SizeToAllocate = sizeInBytes + alignment;
	}

	// Work out what size block is needed and allocate one
	const uint64_t UnitSize = sizeToUnitSize(SizeToAllocate);
	const uint32_t Order    = unitSizeToOrder(UnitSize);
	const uint64_t Offset   = allocateBlockRecursiveBuddyAllocator(Order); // This is the offset in MinBlockSize units

	const uint64_t AllocSize             = orderToUnitSize(Order) * m_MinBlockSize;
	const uint64_t AllocationBlockOffset = Offset * m_MinBlockSize;
	uint64_t Padding = 0;

	if (alignment != 0 && AllocationBlockOffset % alignment != 0)
	{
		uint64_t AlignedBlockOffset = HashInternal::AlignArbitrary(AllocationBlockOffset, alignment);
		Padding                     = AlignedBlockOffset - AllocationBlockOffset;

		ASSERT((Padding + sizeInBytes) <= AllocSize)
	}

	m_TotalSizeUsed += AllocSize;

#if defined(_DEBUG)

	m_Debug_SpaceUsed -= (uint32_t)AllocSize;
	m_Debug_InternalFragmentation -= (uint32_t)Padding;

	if (m_Debug_PeakUsage < m_Debug_SpaceUsed)
	{
		m_Debug_PeakUsage = m_Debug_SpaceUsed;
	}
#endif

	const uint64_t AlignedOffsetFromResourceBase = AllocationBlockOffset + Padding;
	ASSERT((AlignedOffsetFromResourceBase % (uint64_t)alignment) == 0);

	// Setup the info that this allocator
	custom::D3D12BuddyAllocatorPrivateData& PrivateData = resourceLocation.GetBuddyAllocatorPrivateData();
	PrivateData.Order  = (uint32_t)Order;
	PrivateData.Offset = (uint32_t)Offset;

	resourceLocation.SetType(custom::D3D12ResourceLocation::eResourceLocationType::eSubAllocation);
	resourceLocation.SetAllocator((D3D12BuddyAllocator*)this);
	resourceLocation.SetResourceSize(sizeInBytes);

	if (m_eAllocationStrategy == D3D12BuddyAllocator::eAllocationStaretgy::eCommittedResource)
	{
		resourceLocation.SetOffsetFromBaseOfResource(AlignedOffsetFromResourceBase);
		resourceLocation.SetResource(m_pBackingResource);
		resourceLocation.SetGPUVirtualAddress(m_pBackingResource->GetGPUVirtualAddress() + AlignedOffsetFromResourceBase);

		if (D3D12Util::IsCPUWritable(m_kHeapType))
		{
			resourceLocation.SetMappedBaseAddress((uint8_t*)m_pBackingResource->GetCPUVirtualAddress() + AlignedOffsetFromResourceBase);
		}
	}
	else if (m_eAllocationStrategy == D3D12BuddyAllocator::eAllocationStaretgy::ePlacedResource)
	{
		// Place resources are intialized elsewhere
	}
	else
	{
		ASSERT(false);
	}
}

bool D3D12BuddyAllocator::canAllocateBuddyAllocator(uint64_t sizeInBytes, uint32_t alignment)
{
	if (m_TotalSizeUsed == m_MaxBlockSize)
	{
		return false;
	}

	uint64_t sizeToAllocate = sizeInBytes;

	// If the alignment doesn't match the block size
	if (alignment != 0 && m_MinBlockSize % alignment != 0)
	{
		sizeToAllocate = sizeInBytes + alignment;
	}

	uint64_t blockSize = m_MaxBlockSize;

	/*
	for (long i = long(m_FreeBlocks.size() - 1); 0 <= i; --i)
	{
		if (m_FreeBlocks[i].size() && sizeToAllocate <= blockSize)
		{
			return true;
		}

		// Halve the block size;
		blockSize /= 2;

		if (blockSize < sizeToAllocate)
		{
			return false;
		}
	}
	*/
	for (auto itr = m_FreeBlocks.rbegin(); itr != m_FreeBlocks.rend(); ++itr)
	{
		if ((sizeToAllocate <= itr->size()) && blockSize)
		{
			return true;
		}

		blockSize >>= 1;

		if (blockSize < sizeToAllocate)
		{
			return false;
		}
	}

	return false;
}
void D3D12BuddyAllocator::deallocateBlockRecursiveBuddyAllocator(uint32_t offset, uint32_t order)
{
	// See if the buddy block is free  
	const uint32_t size  = orderToUnitSize(order); // 2^order
	const uint32_t buddy = getBuddyOffset(offset, size); // offset ^ size

	const std::set<uint32_t>::iterator it = m_FreeBlocks[order].find(buddy);

	if (it != m_FreeBlocks[order].end())
	{
		// Deallocate merged blocks
		deallocateBlockRecursiveBuddyAllocator((offset < buddy) ? offset : buddy, order + 1);

		// Remove the buddy from the free list
		m_FreeBlocks[order].erase(*it);
	}
	else
	{
		// Add the block to the free list
		m_FreeBlocks[order].insert(offset);
	}
}
void D3D12BuddyAllocator::deallocateInternalBuddyAllocator(D3D12BuddyAllocator::RetiredBlock& retiredBlock)
{
	deallocateBlockRecursiveBuddyAllocator(retiredBlock.Offset, retiredBlock.Order);

	const uint32_t Size = uint32_t(orderToUnitSize(retiredBlock.Order) * m_MinBlockSize);

#if defined(_DEBUG)
	m_Debug_SpaceUsed -= Size;
	m_Debug_InternalFragmentation -= retiredBlock.Debug_InternalFragmentation;
#endif

	m_TotalSizeUsed -= Size;

	if (m_eAllocationStrategy == eAllocationStaretgy::ePlacedResource)
	{
		// Release the resource, SafeRelease()
		ASSERT(retiredBlock.pPlacedResource != nullptr);
		if(!retiredBlock.pPlacedResource->DecrementReference())
		{
			delete retiredBlock.pPlacedResource;
		}
		retiredBlock.pPlacedResource = nullptr;
	}
}

void D3D12BuddyAllocator::ReleaseAllResources()
{
#if defined(_DEBUG)
	m_Debug_NumBlocksInDeferredDeletionQueue -= (uint32_t)m_DeferredDeletionPlacedResourceQueue.size();
#endif

	for (RetiredBlock& Block : m_DeferredDeletionPlacedResourceQueue)
	{
		deallocateInternalBuddyAllocator(Block);
	}

	m_DeferredDeletionPlacedResourceQueue.clear();
	m_DeferredDeletionPlacedResourceQueue.shrink_to_fit();

	if (m_pBackingResource)
	{
		// Warn!!
		// ASSERT(m_pBackingResource->GetRefCount() == 1 || g_NumExplicitGPUsForRendering > 1);
		m_pBackingHeap->Destroy();
		m_pBackingResource = nullptr;
	}

	if (m_pBackingHeap)
	{
		m_pBackingHeap->Destroy();
		m_pBackingHeap = nullptr;
	}
}
bool D3D12BuddyAllocator::DebugAllocateBuddyAllocator(const uint64_t sizeInBytes, const uint32_t alignment, custom::D3D12ResourceLocation& resourceLocation)
{
	custom::Scoped_CS scoped_CS(m_RAII_CS);

	if (canAllocateBuddyAllocator(sizeInBytes, alignment))
	{
		allocateBuddyAllocator(sizeInBytes, alignment, resourceLocation);
		return true;
	}
	else
	{
#if defined(_DEBUG)
		m_Debug_FailedAllocationSpace -= (uint32_t)sizeInBytes;
#endif
		return false;
	}
}
void D3D12BuddyAllocator::DeallocateBuddyAllocator(custom::D3D12ResourceLocation& resourceLocation, uint64_t fenceValue)
{
	ASSERT(IsOwner(resourceLocation));
	// Blocks are cleaned up async so need a lock
	custom::Scoped_CS scoped_CS(m_RAII_CS);

	// FD3D12Adapter* Adapter = GetParentDevice()->GetParentAdapter();

	// COMMAND_LIST_TYPE_DIRECT, COMPUTE, COPY 인지 어떻게 확인?
	// FD3D12Fence& FrameFence = Adapter->GetFrameFence();

	m_DeferredDeletionPlacedResourceQueue.push_back({});
	RetiredBlock& LastDeferredDeletionBlock = m_DeferredDeletionPlacedResourceQueue.back();

	LastDeferredDeletionBlock.FenceValue = fenceValue;

	LastDeferredDeletionBlock.Order  = resourceLocation.GetBuddyAllocatorPrivateData().Order;
	LastDeferredDeletionBlock.Offset = resourceLocation.GetBuddyAllocatorPrivateData().Offset;

#if defined(_DEBUG)
	LastDeferredDeletionBlock.Debug_InternalFragmentation
		= orderToUnitSize(LastDeferredDeletionBlock.Order) * (uint32_t)m_MinBlockSize - (uint32_t)resourceLocation.GetResourceSize();

	--m_Debug_NumBlocksInDeferredDeletionQueue;
#endif

	if (resourceLocation.GetResource()->GetPlacedManagedHeap())
	{
		LastDeferredDeletionBlock.pPlacedResource = resourceLocation.GetResource();
	}
}

#pragma endregion D3D12_BUDDY_ALLOCATOR

D3D12MultiBuddyAllocator::D3D12MultiBuddyAllocator
(
	custom::D3D12GPUNode* const pOwnedGPUNode,
	const D3D12_NODE_MASK creationMask,
	const D3D12_NODE_MASK visibleNodesMask,
	const D3D12BuddyAllocator::eAllocationStaretgy eAllocationStrategy,
	const D3D12_HEAP_TYPE heapType,
	const D3D12_HEAP_FLAGS heapFlags,
	const D3D12_RESOURCE_FLAGS resourceFlags,
	const eAllocatorType eAllocatorType,
	const uint64_t maxSizeForPooling,
	const uint64_t maxBlockSize,
	const uint64_t minBlockSize
)
	:
	BaseD3D12ResourceAllocator(pOwnedGPUNode, creationMask, visibleNodesMask, heapType, resourceFlags, maxSizeForPooling),
	m_eAllocationStrategy(eAllocationStrategy),
	m_eAllocatorType(eAllocatorType),
	m_HeapFlags(heapFlags),
	m_MaxBlockSize(maxBlockSize),
	m_MinBlockSize(minBlockSize),
	m_LastFenceValue(0ull)
{
	ASSERT(pOwnedGPUNode);
}

D3D12MultiBuddyAllocator::~D3D12MultiBuddyAllocator()
{
	DestroyMultiBuddyAllocator(m_LastFenceValue);
}

D3D12BuddyAllocator* D3D12MultiBuddyAllocator::addAllocatorMultiBuddyAllocator()
{
	return new D3D12BuddyAllocator
	(
		m_pOwnedGPUNode,
		m_GPUCreationMask,
		m_GPUVisibilityMask,
		m_eAllocationStrategy,
		m_kHeapType,
		m_HeapFlags,
		m_ResourceFlags,
		m_eAllocatorType,
		m_kMaximumAllocationSizeForPooling,
		m_MaxBlockSize,
		m_MinBlockSize
	);
}

void D3D12MultiBuddyAllocator::InitializeMultiBuddyAllocator()
{
	m_pBuddyAllocators.reserve(4);

	m_pBuddyAllocators.push_back
	(
		new D3D12BuddyAllocator
		(
			m_pOwnedGPUNode,
			m_GPUCreationMask,
			m_GPUVisibilityMask,
			m_eAllocationStrategy,
			m_kHeapType,
			m_HeapFlags,
			m_ResourceFlags,
			m_eAllocatorType,
			m_kMaximumAllocationSizeForPooling,
			m_MaxBlockSize,
			m_MinBlockSize
		)
	);
}

void D3D12MultiBuddyAllocator::DestroyMultiBuddyAllocator(uint64_t fenceValue)
{
	UpdateFenceValue(fenceValue);
	ReleaseAllBuddyAllocatorResources(fenceValue);
}

bool D3D12MultiBuddyAllocator::TryAllocateMultiBuddyAllocator(uint64_t sizeInBytes, uint32_t alignment, custom::D3D12ResourceLocation& resourceLocation)
{
	custom::Scoped_CS ScopedCS(m_RAII_CS);

	for (size_t i = 0; i < m_pBuddyAllocators.size(); ++i)
	{
		if (m_pBuddyAllocators[i]->DebugAllocateBuddyAllocator(sizeInBytes, alignment, resourceLocation))
		{
			return true;
		}
	}

	m_pBuddyAllocators.push_back
	(
		new D3D12BuddyAllocator
		(
			m_pOwnedGPUNode,
			m_GPUCreationMask,
			m_GPUVisibilityMask,
			m_eAllocationStrategy,
			m_kHeapType,
			m_HeapFlags,
			m_ResourceFlags,
			m_eAllocatorType,
			m_kMaximumAllocationSizeForPooling,
			m_MaxBlockSize,
			m_MinBlockSize
		)
	);

	return m_pBuddyAllocators.back()->DebugAllocateBuddyAllocator(sizeInBytes, alignment, resourceLocation);
}
void D3D12MultiBuddyAllocator::DeallocateMultiBuddyAllocator(custom::D3D12ResourceLocation& resourceLocation)
{
	ASSERT(false); // Not yet.
}

void D3D12MultiBuddyAllocator::CleanUpAllBuddyAllocators(uint64_t fenceValue)
{
	m_RAII_CS.EnterCS();

	UpdateFenceValue(fenceValue);

	for (auto*& Allocator : m_pBuddyAllocators)
	{
		Allocator->CleanUpAllocations(fenceValue);
	}

	// Trim empty allocators
	for (long i = (long)(m_pBuddyAllocators.size() - 1); 0 <= i; --i)
	{
		if (m_pBuddyAllocators[i]->IsFreeBlocksEmpty())
		{
			m_pBuddyAllocators[i]->CleanUpAllocations(fenceValue);
			delete(m_pBuddyAllocators[i]);
			// m_pBuddyAllocators.erase(m_pBuddyAllocators.begin() + i);
		}
	}

	m_pBuddyAllocators.clear();
	m_pBuddyAllocators.shrink_to_fit();

	m_RAII_CS.LeaveCS();
}
void D3D12MultiBuddyAllocator::ReleaseAllBuddyAllocatorResources(uint64_t fenceValue)
{
	m_RAII_CS.EnterCS();

	UpdateFenceValue(fenceValue);

	for (long i = (long)(m_pBuddyAllocators.size() - 1); 0 <= i; --i)
	{
		m_pBuddyAllocators[i]->CleanUpAllocations(fenceValue);
		delete(m_pBuddyAllocators[i]);
	}

	m_pBuddyAllocators.clear();
	m_pBuddyAllocators.shrink_to_fit();

	m_RAII_CS.LeaveCS();
}
inline bool D3D12BuddyAllocator::IsOwner(const custom::D3D12ResourceLocation& resourceLocation)
{
	return resourceLocation.GetAllocator() == (D3D12BuddyAllocator*)this;
}