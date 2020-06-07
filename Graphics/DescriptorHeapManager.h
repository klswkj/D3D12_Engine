#pragma once
#include "stdafx.h"
#include "Device.h"
//
// DescriptorHeapManager ¦¡¦¡ DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES]
// 

namespace device
{
    extern ID3D12Device* g_pDevice;
    extern uint32_t g_DescriptorSize[];
}

class DescriptorHeapAllocator
{
public:
    DescriptorHeapAllocator(D3D12_DESCRIPTOR_HEAP_TYPE Type)
        : m_type(Type), m_pCurrentHeap(nullptr), m_descriptorSize(0), m_remainingFreeHandles(0)
    {
        m_currentHandle.ptr = 0;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32_t Count)
    {
        if (m_pCurrentHeap == nullptr || m_remainingFreeHandles < Count)
        {
            m_pCurrentHeap = RequestNewHeap(m_type);
            m_currentHandle = m_pCurrentHeap->GetCPUDescriptorHandleForHeapStart();
            m_remainingFreeHandles = sm_NumDescriptorsPerHeap;

            if (m_descriptorSize == 0)
            {
                m_descriptorSize = device::g_DescriptorSize[m_type];
            }
        }

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = m_currentHandle;
        m_currentHandle.ptr += Count * m_descriptorSize;
        m_remainingFreeHandles -= Count;
        return CPUHandle;
    }

    static void DestroyAll(void)
    {
        sm_descriptorHeapPool.clear();
    }

protected:
    ID3D12DescriptorHeap* RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type)
    {
        std::lock_guard<std::mutex> LockGuard(sm_AllocationMutex);

        D3D12_DESCRIPTOR_HEAP_DESC Desc;
        Desc.Type = Type;
        Desc.NumDescriptors = sm_NumDescriptorsPerHeap;
        Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        Desc.NodeMask = 1;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pHeap;
        ASSERT_HR(device::g_pDevice->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pHeap)));
        sm_descriptorHeapPool.emplace_back(pHeap);
        return pHeap.Get();
    }

protected:
    static constexpr uint32_t sm_NumDescriptorsPerHeap = 256;
    static std::mutex sm_AllocationMutex;

    static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> sm_descriptorHeapPool;

    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
    ID3D12DescriptorHeap* m_pCurrentHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_currentHandle;
    uint32_t m_descriptorSize;
    uint32_t m_remainingFreeHandles;
};

class DescriptorHeapManager
{
public:
    D3D12_CPU_DESCRIPTOR_HANDLE Allocate(D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t Count = 1)
    {
        return m_descriptorHeapAllocator[Type].Allocate(Count);
    }

    void DestroyAll(void)
    {
        for (size_t i{ 0 }; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            m_descriptorHeapAllocator[i].DestroyAll();
        }
    }

private:
    // class DescriptorHeapAllocator Definition.

    DescriptorHeapAllocator m_descriptorHeapAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
    {
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV
    };
};