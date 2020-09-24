#pragma once
#include "Device.h"
//
// DescriptorHeapManager ¦¡¦¡ DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES]
// 

// namespace-device
// ID3D12Device* g_pDevice;
// uint32_t g_DescriptorSize[];


class DescriptorHeapAllocator
{
public:
    DescriptorHeapAllocator(D3D12_DESCRIPTOR_HEAP_TYPE Type)
        : m_type(Type), m_pCurrentHeap(nullptr), m_descriptorSize(0), m_remainingFreeHandles(0)
    {
        m_currentHandle.ptr = 0;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32_t Count);

    static void DestroyAll(void);

protected:
    static ID3D12DescriptorHeap* RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type);

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

// std::mutex DescriptorHeapAllocator::sm_AllocationMutex;
// std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> DescriptorHeapAllocator::sm_descriptorHeapPool;

class DescriptorHeapManager
{
public:
    D3D12_CPU_DESCRIPTOR_HANDLE Allocate(D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t Count = 1)
    {
        return m_descriptorHeapAllocator[Type].Allocate(Count);
    }

    void DestroyAll(void)
    {
        for (size_t i= 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
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