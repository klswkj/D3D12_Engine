#pragma once

namespace custom
{
	class D3D12GPUNode;
	class ManagedD3D12Heap;
	class GPUResource;
	class D3D12ResourceLocation;
	struct D3D12BuddyAllocatorPrivateData;
}

enum class eAllocatorType
{
	DefaultBufferAllocator = 0,
	DynamicHeapAllocator,
	TextureAllocator,
	DefaultBufferAllocatorFullResources
};

class BaseD3D12ResourceAllocator
{
	// Concrete Allocator
	friend class D3D12CommonResourceAllocator; 
public:
	explicit BaseD3D12ResourceAllocator
	(
		// ID3D12Device* const        pDevice,
		custom::D3D12GPUNode* const pOwnedGPUNode,
		const D3D12_NODE_MASK      creationMask,
		const D3D12_NODE_MASK      visibleNodesMask,
		const D3D12_HEAP_TYPE      heapType,
		const D3D12_RESOURCE_FLAGS resourceFlags,
		const uint64_t             maxSizeForPooling = _32MB
	)
		:
		// m_pGlobalDevice(pDevice),
		m_pOwnedGPUNode(pOwnedGPUNode),
		m_GPUCreationMask(creationMask),
		m_GPUVisibilityMask(visibleNodesMask),
		m_ResourceFlags(resourceFlags),
		m_kHeapType(heapType),
		m_kMaximumAllocationSizeForPooling(maxSizeForPooling)
	{
	}
	~BaseD3D12ResourceAllocator() {}

protected:
	custom::D3D12GPUNode*      m_pOwnedGPUNode;
	// ID3D12Device* const        m_pGlobalDevice;
	const D3D12_NODE_MASK      m_GPUCreationMask;
	const D3D12_NODE_MASK      m_GPUVisibilityMask;
	const uint64_t             m_kMaximumAllocationSizeForPooling;
	const D3D12_RESOURCE_FLAGS m_ResourceFlags;
	const D3D12_HEAP_TYPE      m_kHeapType;
	custom::RAII_CS            m_RAII_CS;
};

// Associated with NodeMask
// Only must set Exaclty on bit NodeMask. 
// 1. D3D12CommandQueue
// 2. D3D12CommandList
// 3. D3D12DescritptorHeap
// 4. QueryHeap
// Can set Multiple bitset.
// 1. GraphicsPSO
// 2. ComputePSO
// 3. RootSignature
// 4. CommandSignature

// Both(CreationNodeMask, VisibleNodeMask)
// 1. D3D12_PROPERTIES_HEAP
// can get Only VisibleMask
// 1. GetResourceAllocationINFO
// can get Only NodeMask
// 1. GetCustmHeapProperties

// https://www.cs.fsu.edu/~engelen/courses/COP402003/p827.pdf
// to be member of CustomD3D12Adapter
// For VertexBuffers, IndexBuffers, StructuredBuffers and AccelerationBuffers(TBA[added])
class D3D12BuddyAllocator : public BaseD3D12ResourceAllocator
{
public:
	// TODO : change to Raw Type(D3D12_RESOURCE_TYPE)
	enum class eAllocationStaretgy
	{
		eCommittedResource = 0, // SRV && D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE
		ePlacedResource = 1     // UAV -> If Allow UNORDRED_ACCESS
	};

	explicit D3D12BuddyAllocator
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
		const uint64_t minBlockSize = _64KB // 64 * 1024
	);

	~D3D12BuddyAllocator()
	{
		// ?
		// CleanUpMultiAllocator();
	}

	void InitializeBuddyAllocator();
	void CleanUpAllocations(uint64_t fenceValue);

	bool DebugAllocateBuddyAllocator(const uint64_t sizeInBytes, const uint32_t alignment, custom::D3D12ResourceLocation& resourceLocation);
	void DeallocateBuddyAllocator(custom::D3D12ResourceLocation& resourceLocation, uint64_t fenceValue);
	
	void ReleaseAllResources();
	void ResetBuddyAllocator();
	// void DumpAllocatorStats(class FOutputDevice& Ar);

	inline bool IsFreeBlocksEmpty()
	{
		return m_FreeBlocks[m_MaxOrder].size() == 0ul;
	}

	inline uint64_t GetTotalSizeUsed() const { return m_TotalSizeUsed; }
	uint64_t GetAllocationOffsetInBytes(const custom::D3D12BuddyAllocatorPrivateData& allocatorData) const;

	inline custom::ManagedD3D12Heap* GetBackingHeap()
	{ 
		ASSERT(m_eAllocationStrategy == eAllocationStaretgy::ePlacedResource); 
		// m_pBackingHeap.GetReference();
		return m_pBackingHeap;
	}

	inline bool IsOwner(const custom::D3D12ResourceLocation& resourceLocation);

private:
	struct RetiredBlock
	{
		// ID3D12Resource*                pPlacedResource;
		custom::GPUResource* pPlacedResource;
		uint64_t             FenceValue;
		uint32_t             Order;
		uint32_t             Offset;
#if defined(_DEBUG)
		// Padding is only need in debug builds to keep track of internal fragmentation for stats.
		uint32_t Debug_InternalFragmentation;
#endif
	};

	inline uint64_t sizeToUnitSize(uint64_t size) const
	{
		return (size + (m_MinBlockSize - 1ull)) / m_MinBlockSize;
	}
	inline uint32_t unitSizeToOrder(const uint64_t size) const
	{
		unsigned long Result;
		_BitScanReverse64(&Result, size + size - 1); // ceil(log2(size))
		return (uint32_t)Result;
	}
	inline uint32_t getBuddyOffset(const uint32_t& offset, const uint32_t& size)
	{
		return offset ^ size;
	}

	inline uint32_t orderToUnitSize(uint32_t order) const { return ((uint32_t)1) << order; }
	uint32_t allocateBlockRecursiveBuddyAllocator(uint32_t order);
	bool canAllocateBuddyAllocator(uint64_t sizeInBytes, uint32_t alignment);
	void allocateBuddyAllocator(const uint64_t sizeInBytes, const uint32_t alignment, custom::D3D12ResourceLocation& resourceLocation);

	void deallocateBlockRecursiveBuddyAllocator(uint32_t offset, uint32_t order);
	void deallocateInternalBuddyAllocator(D3D12BuddyAllocator::RetiredBlock& retireBlock);

protected:
	const uint64_t            m_MaxBlockSize;
	const uint64_t            m_MinBlockSize;
	const D3D12_HEAP_FLAGS    m_HeapFlags;
	const eAllocatorType      m_eAllocatorType;
	const eAllocationStaretgy m_eAllocationStrategy;

	custom::ManagedD3D12Heap* m_pBackingHeap;
	// ID3D12Resource*           m_pBackingResource;
	custom::GPUResource*      m_pBackingResource;

private:
	std::vector<D3D12BuddyAllocator::RetiredBlock> m_DeferredDeletionPlacedResourceQueue;
	std::vector<std::set<uint32_t>>                m_FreeBlocks; // Array of set<uint32>
	uint64_t m_MaxOrder;
	uint64_t m_TotalSizeUsed;

#if defined(_DEBUG)
	uint32_t m_Debug_SpaceUsed;
	uint32_t m_Debug_InternalFragmentation;
	uint32_t m_Debug_NumBlocksInDeferredDeletionQueue;
	uint32_t m_Debug_PeakUsage;
	uint32_t m_Debug_FailedAllocationSpace;
#endif
};

// Sizer<sizeof(BaseD3D12ResourceAllocator)> SizerBaseD3D12ResourceAllocator; // Size : 72 - (Debug : 24) => 48
// Sizer<sizeof(D3D12BuddyAllocator)>        SizerD3D12BuddyAllocator;        // Size : 144
// Sizer<sizeof(std::vector<uint64_t>)>      Sizeruint64_tVector;             // Size : 32
// Sizer<sizeof(D3D12_HEAP_FLAGS)>           SizerD3D12_HEAP_FLAGS;           // Size : 4

class D3D12MultiBuddyAllocator : public BaseD3D12ResourceAllocator
{
public:
	D3D12MultiBuddyAllocator
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
		const uint64_t minBlockSize = _64KB // 64 * 1024
	);

	~D3D12MultiBuddyAllocator();

	void InitializeMultiBuddyAllocator();
	void DestroyMultiBuddyAllocator(uint64_t fenceValue);

	bool TryAllocateMultiBuddyAllocator(uint64_t sizeInBytes, uint32_t alignment, custom::D3D12ResourceLocation& resourceLocation);
	void DeallocateMultiBuddyAllocator(custom::D3D12ResourceLocation& resourceLocation);

	void CleanUpAllBuddyAllocators(uint64_t fenceValue);
	void ReleaseAllBuddyAllocatorResources(uint64_t fenceValue);

	const D3D12BuddyAllocator::eAllocationStaretgy GetAllocationStrategy() const { return m_eAllocationStrategy; }
	void UpdateFenceValue(uint64_t fenceValue) { ASSERT(fenceValue <= m_LastFenceValue); m_LastFenceValue = fenceValue; }

	void DumpAllocatorStats() 
	{
		ASSERT(false); // Not yet.
	}
protected:
	D3D12BuddyAllocator* addAllocatorMultiBuddyAllocator();

protected:
	const D3D12BuddyAllocator::eAllocationStaretgy m_eAllocationStrategy;

	const D3D12_HEAP_FLAGS         m_HeapFlags;
	const eAllocatorType           m_eAllocatorType;
	const uint64_t                 m_MaxBlockSize;
	const uint64_t                 m_MinBlockSize;
	uint64_t                       m_LastFenceValue;

	std::vector<D3D12BuddyAllocator*> m_pBuddyAllocators;
};