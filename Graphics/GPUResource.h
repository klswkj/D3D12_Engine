// GPUResource Interitance structure.                                                           Graphicaly
//                                                                                                  ^
//    GPUResource                                                           Programmatically    <式式式托式式式> User-Friendly 
//         弛                                                                                       
//       忙式扛式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式忖
//    UAVBuffer   LinearAllocationPage         PixelBuffer          Texture
//        弛                                         弛
//        弛                                         戍式式式式式式式式式式式式式式式式式式式忖
//        弛                                    DepthBuffer         ColorBuffer
//        弛                                         弛
//        弛                                    ShadowBuffer
//        戍式式式式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式忖
// ByteAddressBuffer        ReadyBackBuffer       TypedBuffer       StructuredBuffer   TypedBuffer
//        弛
// DispatchIndirectBuffer

#pragma once
#include "stdafx.h"

namespace custom
{
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
            m_numSubResource(0u),
            m_pResource(nullptr),
            m_GPUVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
            m_userAllocatedMemory(nullptr),
            m_currentStates({}),
            m_pendingStates({})
        {
        }

        explicit GPUResource
        (
            ID3D12Device* const pDevice, ID3D12Resource* const pResource, const uint32_t numSubResource, 
            const D3D12_RESOURCE_STATES* const pCurrentStates, const D3D12_RESOURCE_STATES* const pPendingStates = nullptr
        )
            :
            m_numSubResource(numSubResource),
            m_GPUVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
            m_userAllocatedMemory(nullptr),
            m_pResource(pResource)
        {
            m_currentStates.resize(numSubResource);
            m_pendingStates.resize(numSubResource);

            for (size_t i = 0; i < numSubResource; ++i)
            {
                m_currentStates[i] = *(pCurrentStates + i);
                m_pendingStates[i] = pPendingStates ? *(pPendingStates + i) : D3D12_RESOURCE_STATES(-1);
            }
        }

        virtual ~GPUResource() = default;

        virtual void Destroy()
        {
            SafeRelease(m_pResource);
            m_GPUVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
            if (m_userAllocatedMemory != nullptr)
            {
                VirtualFree(m_userAllocatedMemory, 0, MEM_RELEASE);
                m_userAllocatedMemory = nullptr;
            }
        }

        void ExplicitPromoteTransition(UINT subResourceIndexBitFlag, D3D12_RESOURCE_STATES promoteState)
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

        ID3D12Resource* operator->()
        { 
            return m_pResource; 
        }
        const ID3D12Resource* operator->() const 
        { 
            return m_pResource; 
        }
        ID3D12Resource* GetResource() const
        { 
            return m_pResource;
        }
        const ID3D12Resource* GetConstResource() const 
        { 
            return m_pResource; 
        }
        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const 
        { 
            return m_GPUVirtualAddress; 
        }
        D3D12_RESOURCE_STATES GetResourceState(const uint8_t subResourceIndex) const
        {
            ASSERT((size_t)subResourceIndex < m_numSubResource)
            return m_currentStates[subResourceIndex];
        }
        D3D12_RESOURCE_STATES GetPendingState(const uint8_t subResourceIndex) const
        {
            ASSERT((size_t)subResourceIndex < m_numSubResource)
            return m_pendingStates[subResourceIndex];
        }
    protected:
        uint32_t m_numSubResource;
        std::vector<D3D12_RESOURCE_STATES> m_currentStates;
        std::vector<D3D12_RESOURCE_STATES> m_pendingStates;
        ID3D12Resource*           m_pResource; // Using with ResourceBarrier synchronization.
        D3D12_GPU_VIRTUAL_ADDRESS m_GPUVirtualAddress;

        // When using VirtualAlloc() to allocate memory directly, 
        // record the allocation here so that it can be freed.  
        // The GpuVirtualAddress may be offset from the true allocation start.
        void* m_userAllocatedMemory;
    };
}