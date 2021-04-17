// GPUResource Inheritance structure.                                                           Graphicaly
//                                                                                                  ^
//    GPUResource                                                           Programmatically    <───┼───> User-Friendly 
//         │                                                                                       
//       ┌─┴──────────────┬─────────────────────────┬──────────────────┐
//    UAVBuffer   LinearAllocationPage         PixelBuffer          Texture
//        │                                         │
//        │                                         ├───────────────────┐
//        │                                    DepthBuffer         ColorBuffer
//        │                                         │
//        │                                    ShadowBuffer
//        ├───────────────────────┬────────────────────┬────────────────┬────────────────┐
// ByteAddressBuffer        ReadyBackBuffer       TypedBuffer       StructuredBuffer   TypedBuffer
//        │
// DispatchIndirectBuffer

#pragma once
#include "d3dx12Residency.h"

namespace custom
{
    class ManagedD3D12Heap;

	class GPUResource
	{
        friend class CommandContext;
        friend class GraphicsContext;
        friend class ComputeContext;
        friend class CopyContext;
        friend class ResourceBarrier;
    public:
        explicit GPUResource()
            :
            m_pManagedHeap(nullptr),
            m_pResource(nullptr),
            m_GPUVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
            m_numSubResource(0u),
            m_pResourceVirtualAddress(nullptr),
            m_currentStates({}),
            m_pendingStates({}),
            // m_bDeferedDelete(true),
            m_GPUVisibleNodeMask({}),
            m_pOwningManager(nullptr),
            m_RefCount(0u)
        {
        }

        explicit GPUResource
        (
            ID3D12Resource* const pResource, 
            D3D12_NODE_MASK visibleNodeMask,
            D3DX12Residency::ResidencyManager* const pResidencyManager,
            const D3D12_RESOURCE_STATES pCurrentStates[], 
            const D3D12_RESOURCE_STATES pPendingStates[] = nullptr
        )
            :
            m_pManagedHeap(nullptr),
            m_pResource(pResource),
            m_GPUVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
            m_pResourceVirtualAddress(nullptr),
            m_numSubResource(-1),
            // m_bDeferedDelete(true),
            m_GPUVisibleNodeMask(visibleNodeMask),
            m_pOwningManager(pResidencyManager),
            m_RefCount(0u)
        {
            ASSERT(pResource && pResidencyManager && pCurrentStates);

			{
				D3D12_RESOURCE_DESC ResourceDesc = m_pResource->GetDesc();

				if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
				{
					m_GPUVirtualAddress = pResource->GetGPUVirtualAddress();
				}

				m_numSubResource = D3D12Util::CalcSubResource(ResourceDesc);
			}

            if (pCurrentStates == nullptr)
            {
                return;
            }

            {
                bool bRAS     = false;
                bool bRestRAS = false;

                if (*pCurrentStates == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)
                {
                    bRAS = true;
                }

                for (size_t i = 1ul; i < m_numSubResource; ++i)
                {
                    if (*(pCurrentStates + i) == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)
                    {
                        bRestRAS = true;
                    }
                    else
                    {
                        bRestRAS = false;
                    }

                    ASSERT(bRAS == bRestRAS);
                }
            }

            if (*pCurrentStates != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)
            {
                m_currentStates.resize(m_numSubResource);
                m_pendingStates.resize(m_numSubResource);

                for (size_t i = 0; i < m_numSubResource; ++i)
                {
                    m_currentStates[i] = *(pCurrentStates + i);
                    m_pendingStates[i] = pPendingStates ? *(pPendingStates + i) : D3D12_RESOURCE_STATES(-1);
                }
            }
        }

        virtual ~GPUResource()
        {
            Destroy();
        }
        virtual void Destroy()
        {
            {
                if (m_ResidencyHandle.IsInitialized())
                {
                    m_pOwningManager->EndTrackingObject(&m_ResidencyHandle);
                }
                ::ZeroMemory(&m_ResidencyHandle, sizeof(m_ResidencyHandle));
            }

            SafeRelease(m_pResource);
            m_GPUVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
            if (m_pResourceVirtualAddress != nullptr)
            {
                VirtualFree(m_pResourceVirtualAddress, 0, MEM_RELEASE);
                m_pResourceVirtualAddress = nullptr;
            }
        }

        void ExplicitPromoteTransition(const UINT subResourceIndexBitFlag, const D3D12_RESOURCE_STATES promoteState)
        {
            uint32_t TempSubResourceBitFlag = subResourceIndexBitFlag;
            uint32_t SubResourceIndex       = -1;

            while (::_BitScanForward((unsigned long*)&SubResourceIndex, TempSubResourceBitFlag))
            {
                if (m_numSubResource <= SubResourceIndex)
                {
                    // If SubResourceIndex is D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
                    break;
                }

                TempSubResourceBitFlag ^= (1u << SubResourceIndex);

                ASSERT(m_currentStates[SubResourceIndex] == D3D12_RESOURCE_STATE_COMMON);

                m_currentStates[SubResourceIndex] = promoteState;
            }
        }
        void ExplicitDecayTransition(const UINT subResourceIndexBitFlag, const D3D12_RESOURCE_STATES decayState)
        {
            ASSERT(false); // Not yet.
        }

        inline void* ResourceMap(const UINT subResourceIndex = 0U, const D3D12_RANGE* const pReadRange = nullptr)
        {
            ASSERT(m_pResource);
            ASSERT_HR(m_pResource->Map(subResourceIndex, pReadRange, (void**)&m_pResourceVirtualAddress));

            return m_pResourceVirtualAddress;
        }

        inline void ResourceUnmap(UINT subResourceIndex = 0U)
        {
            ASSERT(m_pResource && m_pResourceVirtualAddress);
            m_pResource->Unmap(subResourceIndex, nullptr);
            m_pResourceVirtualAddress = nullptr;
        }

#pragma region GETTER
        ID3D12Resource* operator->()
        { 
            return m_pResource; 
        }
        const ID3D12Resource* operator->() const 
        { 
            return m_pResource; 
        }
        inline ID3D12Resource* GetResource() const
        { 
            return m_pResource;
        }
        const ID3D12Resource* GetConstResource() const 
        { 
            return m_pResource; 
        }
        inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const 
        { 
            return m_GPUVirtualAddress; 
        }
        inline void* GetCPUVirtualAddress() const
        {
            return m_pResourceVirtualAddress;
        }
        inline D3D12_RESOURCE_STATES GetResourceState(const uint8_t subResourceIndex) const
        {
            ASSERT((size_t)subResourceIndex < m_numSubResource)
            return m_currentStates[subResourceIndex];
        }
        inline D3D12_RESOURCE_STATES GetPendingState(const uint8_t subResourceIndex) const
        {
            ASSERT((size_t)subResourceIndex < m_numSubResource)
            return m_pendingStates[subResourceIndex];
        }

        // Maybe not PlacedResource.
        inline ManagedD3D12Heap*        GetPlacedManagedHeap()
        {
            return m_pManagedHeap;
        }
        D3DX12Residency::ManagedObject* GetResidencyHandle();
#pragma endregion GETTER
        // 이 부분 ManagedD3D12Heap랑 겹치는데, 상속으로 만들고 싶다.
        void BeginTrackingResidency();
        void UpdateResidency(D3DX12Residency::ResidencySet* pResidencySet);

        void SetPlacedManagedHeap(custom::ManagedD3D12Heap* pManagedHeap)
        {
            ASSERT(!m_pManagedHeap);
            m_pManagedHeap = pManagedHeap;
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

    protected:
        // Is Placed Resource(m_pManagedHeap)? or CommittedResource(m_pResource)
        ManagedD3D12Heap*         m_pManagedHeap;
        ID3D12Resource*           m_pResource; // Using with ResourceBarrier synchronization.
        D3D12_GPU_VIRTUAL_ADDRESS m_GPUVirtualAddress;

        // When using VirtualAlloc() to allocate memory directly, 
        // record the allocation here so that it can be freed.  
        // The GpuVirtualAddress may be offset from the true allocation start.
        void* m_pResourceVirtualAddress;

        D3DX12Residency::ManagedObject           m_ResidencyHandle;
        D3DX12Residency::ResidencyManager* const m_pOwningManager;

        D3D12_NODE_MASK                    m_GPUVisibleNodeMask;
        uint32_t                           m_numSubResource;
        std::vector<D3D12_RESOURCE_STATES> m_currentStates;
        std::vector<D3D12_RESOURCE_STATES> m_pendingStates;

        volatile uint32_t m_RefCount;
        // bool     m_bDeferDelete; // D3D12Viewport, FastAllocation(Will be)
    };
}

// Sizer<sizeof(D3DX12Residency::ManagedObject)> SizerManagedObject; // Size : 88
// Sizer<sizeof(custom::GPUResource)>            SizerGPUResource;   // Size : 208 + (Ref) -> 212