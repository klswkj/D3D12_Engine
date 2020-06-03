// GPUResource Interitance structure.
// 
// GPUResource
//      弛
//    忙式扛式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式忖
// UAVBuffer   LinearAllocationPage        PixelBuffer          Texture
//    弛                                         弛
//    弛                                         戍式式式式式式式式式式式式式式式式式式式忖
//    弛                                    DepthBuffer         ColorBuffer
//    弛                                         弛
//    弛                                    ShadowBuffer
//  忙式扛式式式式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式忖
// ByteAddressBuffer    ReadyBackBuffer      TypedBuffer       NestedBuffer   TypedBuffer
//        弛
// DispatchIndirectBuffer

#pragma once
#include "stdafx.h"

namespace custom
{
	class GPUResource
	{
    public:
        GPUResource() :
            m_GPUVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
            m_userAllocatedMemory(nullptr),
            m_currentState(D3D12_RESOURCE_STATE_COMMON),
            m_transitionState((D3D12_RESOURCE_STATES)-1)
        {
        }

        GPUResource(ID3D12Device* pDevice, ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            m_GPUVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
            m_userAllocatedMemory(nullptr),
            m_pResource(pResource),
            m_currentState(CurrentState),
            m_transitionState((D3D12_RESOURCE_STATES)-1)
        {
        }

        virtual void Destroy()
        {
            m_pResource = nullptr;
            m_GPUVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
            if (m_userAllocatedMemory != nullptr)
            {
                VirtualFree(m_userAllocatedMemory, 0, MEM_RELEASE);
                m_userAllocatedMemory = nullptr;
            }
        }

        ID3D12Resource* operator->() 
        { 
            return m_pResource.Get(); 
        }
        const ID3D12Resource* operator->() const 
        { 
            return m_pResource.Get(); 
        }

        ID3D12Resource* GetResource() 
        { 
            return m_pResource.Get(); 
        }

        const ID3D12Resource* GetResource() const 
        { 
            return m_pResource.Get(); 
        }

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const 
        { 
            return m_GPUVirtualAddress; 
        }

    protected:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;
        D3D12_RESOURCE_STATES m_currentState;
        D3D12_RESOURCE_STATES m_transitionState;
        D3D12_GPU_VIRTUAL_ADDRESS m_GPUVirtualAddress;

        // When using VirtualAlloc() to allocate memory directly, 
        // record the allocation here so that it can be freed.  
        // The GpuVirtualAddress may be offset from the true allocation start.
        void* m_userAllocatedMemory;
    };
}