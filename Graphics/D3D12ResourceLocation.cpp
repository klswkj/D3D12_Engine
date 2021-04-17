#include "stdafx.h"
#include "D3D12ResourceLocation.h"
#include "CustomD3D12GPUNode.h"
#include "CustomD3D12Adapter.h"
#include "GPUResource.h"
#include "d3dx12Residency.h"
#include "D3D12ResourceAllocator.h"
#include "D3D12SegListAllocator.h"
#include "GPUResource.h"

namespace custom
{
D3D12ResourceLocation::D3D12ResourceLocation(custom::D3D12GPUNode* pParentGPUNode)
	:
	m_pGPUNode(pParentGPUNode),
	m_ResourceLocationType(eResourceLocationType::eUndefined),
	m_pUnderlyingResource(nullptr),
	m_pResidencyHandle(nullptr),
	m_pBuddyAllocator(nullptr),
	m_pMappedBaseAddress(nullptr),
	m_AllocatorData({}),
	m_GPUVirtualAddress(0),
	m_OffsetFromBaseOfResource(0),
	m_ResourceSize(0ull),
	// m_bTransient(false),
	m_eAllocatorType(eAllocatorType::AT_Unknown)
{
	ASSERT(pParentGPUNode);
	::ZeroMemory(&m_AllocatorData, sizeof(m_AllocatorData));
}

D3D12ResourceLocation::~D3D12ResourceLocation()
{
	ReleaseInnerResource();
}

void D3D12ResourceLocation::ClearInnerInfo()
{
	// Reset members
	m_ResourceLocationType = eResourceLocationType::eUndefined;
	m_pUnderlyingResource = nullptr;
	m_pMappedBaseAddress = nullptr;
	m_GPUVirtualAddress = 0ULL;
	m_pResidencyHandle = nullptr;
	m_ResourceSize = 0ull;

	m_OffsetFromBaseOfResource = 0ull;
	::ZeroMemory(&m_AllocatorData, sizeof(m_AllocatorData));

	m_pBuddyAllocator = nullptr;
	m_eAllocatorType = eAllocatorType::AT_Unknown;
}

STATIC void D3D12ResourceLocation::TransferOwnership(D3D12ResourceLocation& destinationLocation, D3D12ResourceLocation& sourceLocation)
{
	// Clear out the destination
	destinationLocation.ReleaseInnerResource();
	destinationLocation.ClearInnerInfo();

	::std::memmove(&destinationLocation, &sourceLocation, sizeof(D3D12ResourceLocation));

	sourceLocation.ClearInnerInfo();
}

void D3D12ResourceLocation::Swap(D3D12ResourceLocation& Other)
{
	::std::swap(*this, Other);
}

STATIC void D3D12ResourceLocation::Alias(D3D12ResourceLocation& destinationLocation, D3D12ResourceLocation& sourceLocation)
{
	ASSERT(sourceLocation.GetResource() != nullptr);
	destinationLocation.ReleaseInnerResource();
	destinationLocation.ClearInnerInfo();

	std::memmove(&destinationLocation, &sourceLocation, sizeof(D3D12ResourceLocation));

	destinationLocation.SetType(eResourceLocationType::eAliased);
	sourceLocation.SetType(eResourceLocationType::eAliased);

	// Addref the source as another resource location references it
	sourceLocation.GetResource()->AddReference();
}

STATIC void D3D12ResourceLocation::ReferenceNode(custom::D3D12GPUNode* pDestinationGPUNode, D3D12ResourceLocation& destinationLocation, D3D12ResourceLocation& sourceLocation)
{
	ASSERT(sourceLocation.GetResource() != nullptr && pDestinationGPUNode);
	destinationLocation.ReleaseInnerResource();
	destinationLocation.ClearInnerInfo();

	std::memmove(&destinationLocation, &sourceLocation, sizeof(D3D12ResourceLocation));
	destinationLocation.SetType(eResourceLocationType::eNodeReference);

	destinationLocation.m_pGPUNode = pDestinationGPUNode;

	// Addref the source as another resource location references it
	sourceLocation.GetResource()->AddReference();
}

void D3D12ResourceLocation::ReleaseInnerResource()
{
	switch (m_ResourceLocationType)
	{
	case eResourceLocationType::eStandAlone:
	{
		// TODO 3 : Multi-GPU support : because of references, several GPU nodes can refrence the same stand-alone resource.
		ASSERT(m_pUnderlyingResource->GetReferenceCount() == 1); //|| 1 < GNumExplicitGPUsForRendering);
		/*
		if (m_pUnderlyingResource->ShouldDeferDelete())
		{
			m_pUnderlyingResource->DeferDelete();
		}
		else
		{
			// m_pUnderlyingResource->Release();
			delete m_pUnderlyingResource;
			m_pUnderlyingResource = nullptr;
		}
		*/
		if (!m_pUnderlyingResource->DecrementReference())
		{
			delete m_pUnderlyingResource;
			m_pUnderlyingResource = nullptr;
			break;
		}
	}
	case eResourceLocationType::eSubAllocation:
	{
		ASSERT(m_pBuddyAllocator != nullptr);
		if (m_eAllocatorType == eAllocatorType::AT_SegList)
		{
			m_pSegListAllocator->Deallocate
			(
				GetResource(),
				GetSegListAllocatorPrivateData().Offset, (uint32_t)GetResourceSize()
			);
		}
		else
		{
			// static_assert(false, "Check D3D12ResourceLocation::ReleaseResource()");
			m_pBuddyAllocator->DeallocateBuddyAllocator(*this, m_pGPUNode->GetParentAdapter()->GetFrameFenceValue());
		}
		break;
	}
	case eResourceLocationType::eNodeReference: break;
	case eResourceLocationType::eAliased:
	{
		// if (m_pUnderlyingResource->ShouldDeferDelete() && m_pUnderlyingResource->GetReferenceCount() == 1)
		// {
		// 	m_pUnderlyingResource->DeferDelete();
		// }
		// else
		{
			if (!m_pUnderlyingResource->DecrementReference())
			{
				delete m_pUnderlyingResource;
				m_pUnderlyingResource = nullptr;
				break;
			}
		}
		break;
	}
	case eResourceLocationType::eHeapAliased:
	{
		ASSERT(m_pUnderlyingResource->GetReferenceCount() == 1);

		// if (m_pUnderlyingResource->ShouldDeferDelete())
		// {
		// 	m_pUnderlyingResource->DeferDelete();
		// }
		///else
		{
			if (!m_pUnderlyingResource->DecrementReference())
			{
				delete m_pUnderlyingResource;
				m_pUnderlyingResource = nullptr;
				break;
			}
		}
		break;
	}
	case eResourceLocationType::eFastAllocation: break;
	case eResourceLocationType::eUndefined: break;
	default:
		ASSERT(false, "Invalid Resource Location Type.")
		break;
	}
}

// void D3D12ResourceLocation::SetResource
// (
// 	ID3D12Resource* const pResource,
// 	D3DX12Residency::ManagedObject* const pResidencyManagedObject
// )
// {
// 	ASSERT(m_pUnderlyingResource == nullptr);
// 	ASSERT(m_pResidencyHandle == nullptr);
// 
// 	m_pUnderlyingResource = pResource;
// 	m_GPUVirtualAddress   = pResource->GetGPUVirtualAddress();
// 	m_pResidencyHandle    = pResidencyManagedObject;
// }
void D3D12ResourceLocation::SetResource(custom::GPUResource* pGPUResource)
{
	ASSERT(pGPUResource && !m_pUnderlyingResource && !m_pResidencyHandle);

	m_pUnderlyingResource = pGPUResource;
	m_pResidencyHandle    = pGPUResource->GetResidencyHandle();
}
inline void D3D12ResourceLocation::AsStandAlone
(
	const D3D12ResourceLocation::eResourceLocationType eResourceLocationType,
	custom::GPUResource* const                         pGPUResource,
	const uint64_t                                     resourceSizeInBytes
)
{
	SetType(eResourceLocationType);
	// SetResource(pResource, pResidencyManagedObject);
	SetResource(pGPUResource);
	SetResourceSize(resourceSizeInBytes);

	D3D12_HEAP_PROPERTIES HeapProperties = {};
	D3D12_HEAP_FLAGS HeapFlags = {};

	pGPUResource->GetResource()->GetHeapProperties(&HeapProperties, &HeapFlags);

	bool bCPUInaccessible = D3D12Util::IsCPUInaccessible(HeapProperties.Type, &HeapProperties);

	if (!bCPUInaccessible)
	{
		D3D12_RANGE range = { 0UL, (SIZE_T)resourceSizeInBytes };
		void* pCPUVirtualAddress = pGPUResource->ResourceMap(0U, &range);
		SetMappedBaseAddress(pCPUVirtualAddress);
	}
	SetGPUVirtualAddress(pGPUResource->GetGPUVirtualAddress());
	// SetTransient(bInIsTransient);
}

inline void D3D12ResourceLocation::AsHeapAliased
(
	D3D12ResourceLocation::eResourceLocationType eResourceLocationType,
	// ID3D12Resource* const pResource, 
	custom::GPUResource* const pGPUResource,
	uint32_t bufferSize
	//, void* pResourceBaseAddress,  D3DX12Residency::ManagedObject* const pResidencyManagedObject
)
{
	SetType(eResourceLocationType);
	// SetResource(pResource, pResidencyManagedObject);
	SetResource(pGPUResource);
	SetResourceSize(0);

	D3D12_HEAP_PROPERTIES HeapProperties = {};
	D3D12_HEAP_FLAGS HeapFlags = {};
	// pResource->GetHeapProperties(&HeapProperties, &HeapFlags);
	pGPUResource->GetResource()->GetHeapProperties(&HeapProperties, &HeapFlags);

	bool bCPUInaccessible = D3D12Util::IsCPUInaccessible(HeapProperties.Type, &HeapProperties);

	if (!bCPUInaccessible)
	{
		D3D12_RANGE range = { 0UL, (SIZE_T)bufferSize };
		void* pCPUVirtualAddress = pGPUResource->ResourceMap(0U, &range);
		SetMappedBaseAddress(pCPUVirtualAddress);
	}
	SetGPUVirtualAddress(pGPUResource->GetGPUVirtualAddress());
	// SetTransient(bInIsTransient);
}
inline void D3D12ResourceLocation::AsFastAllocation
(
	// ID3D12Resource* pResource,
	custom::GPUResource* pGPUResource,
	uint32_t bufferSize,
	D3D12_GPU_VIRTUAL_ADDRESS GPUBase,
	void* pCPUBase, uint64_t offset,
	D3DX12Residency::ManagedObject* const pResidencyManagedObject,
	bool bMultiFrame // = false
)
{
	if (bMultiFrame)
	{
		// pResource->AddRef();
		SetType(eResourceLocationType::eMultiFrameFastAllocation);
	}
	else
	{
		SetType(eResourceLocationType::eFastAllocation);
	}

	// SetResource(pResource, pResidencyManagedObject);
	SetResource(pGPUResource);
	SetResourceSize(bufferSize);
	SetOffsetFromBaseOfResource(offset);

	if (pCPUBase != nullptr)
	{
		// SetMappedBaseAddress((uint8_t*)pCPUBase + offset);
		SetMappedBaseAddress((void*)((uint8_t*)pCPUBase + offset));
	}
	SetGPUVirtualAddress(GPUBase + offset);
}
}
