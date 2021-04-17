#include "stdafx.h"
#include "D3D12CommonResourceAllocator.h"
#include "CustomD3D12Adapter.h"
#include "CustomD3D12GPUNode.h"
#include "ManagedD3D12Heap.h"
#include "D3D12ResourceAllocator.h"
#include "D3D12ResourceLocation.h"

/*
#define	D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT	( 65536 )
#define	D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT	    ( 4096 )
#define	D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT	    ( 65536 )
#define	D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT	( 4194304 )
#define	D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT	( 256 )
*/

D3D12CommonResourceAllocator::D3D12CommonResourceAllocator
(
	custom::D3D12GPUNode* const pParentGPUNode,
	const D3D12_NODE_MASK       visibleNdoeMask
)
	:
	m_pParentGPUNode(pParentGPUNode),
	m_VisibleNodeMask(visibleNdoeMask),
	m_LastFenceValue(0ull)
{
	ASSERT(pParentGPUNode);
	ASSERT((1 << pParentGPUNode->GetGPUNodeIndexMask().NodeMask) & visibleNdoeMask.NodeMask);
}

D3D12CommonResourceAllocator::~D3D12CommonResourceAllocator()
{
	FreeDefaultBufferPools(m_LastFenceValue);
}

void D3D12CommonResourceAllocator::InitializeAllocator
(
	const D3D12CommonResourceAllocator::eViewType ePoolViewType, 
	const D3D12_RESOURCE_FLAGS                    resourceFlags
)
{
	D3D12MultiBuddyAllocator* pAllocator = nullptr;

	if (ePoolViewType == eViewType::UAV)
	{
		pAllocator = new D3D12MultiBuddyAllocator
		(
			m_pParentGPUNode,
			m_pParentGPUNode->GetGPUNodeIndexMask(),
			m_VisibleNodeMask,
			D3D12BuddyAllocator::eAllocationStaretgy::ePlacedResource,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_HEAP_FLAG_NONE | D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS,
			resourceFlags,
			eAllocatorType::DefaultBufferAllocator,
			DEFAULT_BUFFER_POOL_MAX_ALLOC_SIZE,
			DEFAULT_BUFFER_POOL_SIZE,
			MIN_PLACED_BUFFER_SIZE
		);
	}
	else
	{
		pAllocator = new D3D12MultiBuddyAllocator
		(
			m_pParentGPUNode,
			m_pParentGPUNode->GetGPUNodeIndexMask(),
			m_VisibleNodeMask,
			D3D12BuddyAllocator::eAllocationStaretgy::eCommittedResource,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS,
			resourceFlags,
			eAllocatorType::DefaultBufferAllocator,
			DEFAULT_BUFFER_POOL_MAX_ALLOC_SIZE,
			DEFAULT_BUFFER_POOL_SIZE,
			16u
		);
	}

	ASSERT(pAllocator);

	m_pMultiBuddyAllocator[(uint32_t)ePoolViewType] = pAllocator;
}

void D3D12CommonResourceAllocator::CleanUpMultiAllocator(D3D12CommonResourceAllocator::eViewType multiAllocatorType, size_t fenceValue)
{
	ASSERT(multiAllocatorType < eViewType::Count);
	m_LastFenceValue = fenceValue;
	m_pMultiBuddyAllocator[(size_t)multiAllocatorType]->CleanUpAllBuddyAllocators(fenceValue);
}

bool D3D12CommonResourceAllocator::AllocCommonResource
(
	const D3D12_RESOURCE_DESC&     resourceDesc,
	custom::D3D12ResourceLocation& resourceLocation,
	const eBufferUsageFlags        eBufferUsageFlag, 
	const uint32_t                 align
)
{
	const D3D12CommonResourceAllocator::eViewType PoolIndex = GetBufferPool(resourceDesc.Flags);
	ASSERT(PoolIndex < eViewType::Count);

	D3D12MultiBuddyAllocator* pAllocator = m_pMultiBuddyAllocator[(size_t)PoolIndex];

	if (pAllocator == nullptr)
	{
		InitializeAllocator(PoolIndex, resourceDesc.Flags);
	}

	ID3D12Device* const pParentRawDevice = m_pParentGPUNode->GetParentAdapter()->GetRawDevice();
	ASSERT(pParentRawDevice);

	uint64_t ResourceSizeInBytes = 0ul;
	uint64_t AlignmentForResource = 0ul;
	{
		D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo = pParentRawDevice->GetResourceAllocationInfo(m_VisibleNodeMask.NodeMask, 1, &resourceDesc);
		// See Remark in ID3D12Device::GetResourceAllocationInfo method (d3d12.h).
		if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			ResourceSizeInBytes = resourceDesc.Width;
		}
		else
		{
			ResourceSizeInBytes = ResourceAllocationInfo.SizeInBytes;
			// ResourceSizeInBytes = HashInternal::AlignUp(ResourceAllocationInfo.SizeInBytes, ResourceAllocationInfo.Align);
		}

		AlignmentForResource = ResourceAllocationInfo.Alignment;
	}

	ASSERT(ResourceSizeInBytes && AlignmentForResource && AlignmentForResource == align);

	resourceLocation.ReleaseInnerResource();
	resourceLocation.ClearInnerInfo();

	const bool bWithInPoolingSize = ResourceSizeInBytes < pAllocator->m_kMaximumAllocationSizeForPooling; // && ((Desc.Width % _KB(64)) != 0)

	const D3D12_RESOURCE_STATES InitialState 
		= ((bool)(eBufferUsageFlag & eBufferUsageFlags::BufferUsageFlags_AccelerationStructure)) ? 
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE : D3D12_RESOURCE_STATE_GENERIC_READ;

	// const UINT NumSubResources = (resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? resourceDesc.DepthOrArraySize * resourceDesc.MipLevels : resourceDesc.MipLevels;

	if (bWithInPoolingSize)
	{
		const bool bPlacedResourceStaretgy = (pAllocator->GetAllocationStrategy() == D3D12BuddyAllocator::eAllocationStaretgy::ePlacedResource);
		// Check Proper D3D12 Resource Flags
		{
			if (bPlacedResourceStaretgy)
			{
				// Writeable resources get separate ID3D12Resource* with their own resource state by using placed resources. 
				// Just make sure it's UAV, other flags are free to differ.
				ASSERT((resourceDesc.Flags & pAllocator->m_ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0);
			}
			else
			{
				// Read-only resources get suballocated from big resources, 
				// thus share ID3D12Resource* and resource state with other resources. 
				// Ensure it's suballocated from a resource with identical flags.
				ASSERT(resourceDesc.Flags == pAllocator->m_ResourceFlags);
			}
		}

		if (pAllocator->TryAllocateMultiBuddyAllocator(ResourceSizeInBytes, align, resourceLocation))
		{
			if (bPlacedResourceStaretgy)
			{
				ASSERT(resourceLocation.GetResource() == nullptr);

				custom::ManagedD3D12Heap* pBackingHeap = ((D3D12BuddyAllocator*)resourceLocation.GetAllocator())->GetBackingHeap();
				ASSERT(pBackingHeap);
				
				uint64_t HeapOffset = resourceLocation.GetAllocator()->GetAllocationOffsetInBytes(resourceLocation.GetBuddyAllocatorPrivateData());
				
				ID3D12Resource*      pNewPlacedResource = nullptr;
				custom::GPUResource* pNewGPUResource    = nullptr;

				ASSERT_HR(pParentRawDevice->CreatePlacedResource(pBackingHeap->GetHeap(), HeapOffset, &resourceDesc, InitialState, nullptr, IID_PPV_ARGS(&pNewPlacedResource)));

				pNewGPUResource = new custom::GPUResource
				(
					pNewPlacedResource, 
					m_VisibleNodeMask, 
					m_pParentGPUNode->GetResidencyManager(), 
					&InitialState, 
					nullptr
				);

				ASSERT(pNewPlacedResource && pNewGPUResource);

				pNewGPUResource->SetPlacedManagedHeap(pBackingHeap);
				pNewGPUResource->AddReference();

				resourceLocation.SetResource(pNewGPUResource);
			}

			return true;
		}
	}

	D3D12_HEAP_PROPERTIES HeapProps = {};
	HeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask     = m_pParentGPUNode->GetGPUNodeIndexMask().NodeMask;
	HeapProps.VisibleNodeMask      = m_VisibleNodeMask.NodeMask;

	D3D12_HEAP_FLAGS HeapFlags = D3D12_HEAP_FLAG_NONE;

	if (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
	{
		HeapFlags |= D3D12_HEAP_FLAG_SHARED;
	}

	ID3D12Resource* pNewResource = nullptr;
	ASSERT_HR(pParentRawDevice->CreateCommittedResource(&HeapProps, HeapFlags, &resourceDesc, InitialState, nullptr, IID_PPV_ARGS(&pNewResource)));

	// Allocate Standalone
	custom::GPUResource* pStandAloneResource = new custom::GPUResource
	(
		pNewResource,
		m_VisibleNodeMask,
		m_pParentGPUNode->GetResidencyManager(),
		&InitialState, 
		nullptr
	);

	if (D3D12Util::IsCPUInaccessible(HeapProps.Type))
	{
	    pStandAloneResource->BeginTrackingResidency();
	}

	resourceLocation.AsStandAlone(custom::D3D12ResourceLocation::eResourceLocationType::eStandAlone, pStandAloneResource, ResourceSizeInBytes);

	return false;
}

void D3D12CommonResourceAllocator::FreeDefaultBufferPools(uint64_t fenceValue)
{
	m_LastFenceValue = fenceValue;

	for(size_t i = 0; i < (size_t)eViewType::Count; ++i)
	{
		if (m_pMultiBuddyAllocator[i])
		{
			m_pMultiBuddyAllocator[i]->CleanUpAllBuddyAllocators(fenceValue);

			delete m_pMultiBuddyAllocator[i];
			m_pMultiBuddyAllocator[i] = nullptr;
		}
	}
}