#pragma once
#include "D3D12Util.h"

namespace D3DX12Residency
{
	class ManagedObject;
}

namespace custom
{
	class D3D12GPUNode;
	class GPUResource;
}

class D3D12BuddyAllocator;
class D3D12SegListAllocator;

namespace custom
{
#pragma region PRE_DEFINITION
	struct D3D12BuddyAllocatorPrivateData
	{
		uint32_t Offset = 0u;
		uint32_t Order = 0u;
	};
	struct D3D12BlockAllocatorPrivateData
	{
		uint64_t FenceValue = 0ull;
		uint32_t BucketIndex = 0u;
		uint32_t Offset = 0u;
		ID3D12Resource* pResourceHeap = nullptr;
	};
	struct D3D12SegListAllocatorPrivateData
	{
		uint32_t Offset = 0u;
	};
#pragma endregion PRE_DEFINITION

class D3D12ResourceLocation
{
	friend class D3D12BuddyAllocator;
public:
	enum class eResourceLocationType : uint8_t
	{
		eUndefined = 0,
		eStandAlone,
		eSubAllocation,
		eFastAllocation,
		eMultiFrameFastAllocation,
		eAliased, // Oculus is the only API that uses this
		eNodeReference,
		eHeapAliased,
	};

	enum class eAllocatorType : uint8_t
	{
		AT_Default = 0, // FD3D12BaseAllocatorType
		AT_SegList = 1, // FD3D12SegListAllocator
		AT_Unknown = 0xff
	};

	D3D12ResourceLocation(custom::D3D12GPUNode* pParentGPUNode);
	~D3D12ResourceLocation();

	void ClearInnerInfo();
	void ReleaseInnerResource();

#pragma region SETTERS_GETTERS
	// Setters
	// void        SetResource(ID3D12Resource* const pResource, D3DX12Residency::ManagedObject* const pResidencyManagedObject);
	void SetResource(custom::GPUResource* pGPUResource);
	inline void SetType(eResourceLocationType Value)                          { m_ResourceLocationType = Value; }
	inline void SetAllocator(D3D12BuddyAllocator* pBuddyAllocator)            { m_pBuddyAllocator          = pBuddyAllocator;   m_eAllocatorType = eAllocatorType::AT_Default; }
	inline void SetSegListAllocator(D3D12SegListAllocator* pSegListAllocator) { m_pSegListAllocator        = pSegListAllocator; m_eAllocatorType = eAllocatorType::AT_SegList; }
	inline void SetMappedBaseAddress(void* pMappedBaseAddress)                { m_pMappedBaseAddress       = pMappedBaseAddress; }
	inline void SetGPUVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS GPUAddress)    { m_GPUVirtualAddress        = GPUAddress; }
	inline void SetOffsetFromBaseOfResource(uint64_t offset)                  { m_OffsetFromBaseOfResource = offset; }
	inline void SetResourceSize(uint64_t resourceSize)                        { m_ResourceSize = resourceSize; }

	// Getters
	inline eResourceLocationType GetType()                              const { return m_ResourceLocationType; }
	inline D3D12BuddyAllocator* GetAllocator()                          const { ASSERT(eAllocatorType::AT_Default == m_eAllocatorType); return m_pBuddyAllocator; }
	inline D3D12SegListAllocator* GetSegListAllocator()                 const { ASSERT(eAllocatorType::AT_SegList == m_eAllocatorType); return m_pSegListAllocator; }
	// inline ID3D12Resource* GetResource()                                const { return m_pUnderlyingResource; }
	inline custom::GPUResource* GetResource()                           const { return m_pUnderlyingResource; }
	inline void* GetMappedBaseAddress()                                 const { return m_pMappedBaseAddress; }
	// inline void* GetMappedBaseAddress()                                 const { ASSERT(m_pUnderlyingResource); return m_pUnderlyingResource->ResourceMap(); }
	inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress()             const { return m_GPUVirtualAddress; }
	// inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress()             const { ASSERT(m_pUnderlyingResource); return m_pUnderlyingResource->GetGPUVirtualAddress(); }
	inline uint64_t GetOffsetFromBaseOfResource()                       const { return m_OffsetFromBaseOfResource; }
	inline uint64_t GetResourceSize()                                   const { return m_ResourceSize; }
	inline D3DX12Residency::ManagedObject* GetResidencyHandle()         const { return m_pResidencyHandle; }
	inline D3D12BuddyAllocatorPrivateData& GetBuddyAllocatorPrivateData() const { return m_AllocatorData.BuddyAllocatorPrivateData; }
	inline D3D12BlockAllocatorPrivateData& GetBlockAllocatorPrivateData() const { return m_AllocatorData.BlockAllocatorPrivateData; }
	inline D3D12SegListAllocatorPrivateData& GetSegListAllocatorPrivateData() const { return m_AllocatorData.SegListAllocatorPrivateData; }
#pragma endregion SETTERS_GETTERS

	const inline bool IsValid() const { return m_ResourceLocationType != eResourceLocationType::eUndefined; }

	inline void AsStandAlone
	(
		const D3D12ResourceLocation::eResourceLocationType eResourceLocationType,
		custom::GPUResource* const                         pGPUResource,
		const uint64_t                                     resourceSizeInBytes
	);

	inline void AsHeapAliased
	(
		D3D12ResourceLocation::eResourceLocationType eResourceLocationType,
		// ID3D12Resource* const pResource, 
		custom::GPUResource* const pGPUResource,
		uint32_t bufferSize
		//, void* pResourceBaseAddress,  D3DX12Residency::ManagedObject* const pResidencyManagedObject
	);

	inline void AsFastAllocation
	(
		// ID3D12Resource* pResource,
		custom::GPUResource* pGPUResource,
		uint32_t bufferSize,
		D3D12_GPU_VIRTUAL_ADDRESS GPUBase,
		void* pCPUBase, uint64_t offset,
		D3DX12Residency::ManagedObject* const pResidencyManagedObject,
		bool bMultiFrame = false
	);

	// void SetTransient(bool bInTransient) { m_bTransient = bInTransient; }
	// bool IsTransient() const             { return m_bTransient; }
	void Swap(D3D12ResourceLocation& Other);

public:
	// Oculus API Aliases textures so this allows 2+ resource locations to reference the same underlying resource. 
	// We should avoid this as much as possible as it requires expensive reference counting and
	// it complicates the resource ownership model.
	static void Alias(D3D12ResourceLocation& destinationLocation, D3D12ResourceLocation& sourceLocation);
	static void ReferenceNode(custom::D3D12GPUNode* pDestinationGPUNode, D3D12ResourceLocation& destinationLocation, D3D12ResourceLocation& sourceLocation);

	// Transfers the contents of 1 resource location to another, destroying the original but preserving the underlying resource 
	static void TransferOwnership(D3D12ResourceLocation& destinationLocation, D3D12ResourceLocation& sourceLocation);

private:
	custom::D3D12GPUNode*           m_pGPUNode;
	// ID3D12Resource*                 m_pUnderlyingResource; // to be replace to custom::GPUResource?
	custom::GPUResource*            m_pUnderlyingResource;
	D3DX12Residency::ManagedObject* m_pResidencyHandle;

	// Which allocator this belongs to
	union
	{
		D3D12BuddyAllocator*    m_pBuddyAllocator;
		D3D12SegListAllocator*  m_pSegListAllocator;
	};

	// Union to save memory
	mutable union PrivateAllocatorData
	{
		D3D12BuddyAllocatorPrivateData   BuddyAllocatorPrivateData;
		D3D12BlockAllocatorPrivateData   BlockAllocatorPrivateData;
		D3D12SegListAllocatorPrivateData SegListAllocatorPrivateData;
	} m_AllocatorData;

	// Note: These values refer to the start of this location including any padding 
	// *NOT* the start of the underlying resource
	void*                     m_pMappedBaseAddress;
	D3D12_GPU_VIRTUAL_ADDRESS m_GPUVirtualAddress;
	uint64_t                  m_OffsetFromBaseOfResource;

	// The size the application asked for Residency
	uint64_t m_ResourceSize;

	eResourceLocationType m_ResourceLocationType;
	eAllocatorType        m_eAllocatorType;
};
}
// Sizer<sizeof(D3D12ResourceLocation)> SizerD3D12ResourceLocation; // Size : 96