// GPUResource Interitance structure.
// 
// GPUResource
//      ��
//    ����������������������������������������������������������������������������������������������������������������������������
// UAVBuffer   LinearAllocationPage        PixelBuffer          Texture
//    ��                                         ��
//    ��                                         ������������������������������������������
//    ��                                    DepthBuffer         ColorBuffer
//    ��                                         ��
//    ��                                    ShadowBuffer
//  ��������������������������������������������������������������������������������������������������������������������������������������������������������������������
// ByteAddressBuffer    ReadyBackBuffer      TypedBuffer       NestedBuffer   TypedBuffer
//        ��
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