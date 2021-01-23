#include "stdafx.h"
#include "DescriptorHeapManager.h"

std::mutex DescriptorHeapAllocator::sm_AllocationMutex;
std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> DescriptorHeapAllocator::sm_descriptorHeapPool;

/*static*/ void DescriptorHeapAllocator::DestroyAll(void)
{
    sm_descriptorHeapPool.clear();
}

/*static*/ ID3D12DescriptorHeap* DescriptorHeapAllocator::RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
    std::lock_guard<std::mutex> LockGuard(sm_AllocationMutex);

    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    Desc.Type = Type;
    Desc.NumDescriptors = sm_NumDescriptorsPerHeap;
    Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    Desc.NodeMask = 1;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pHeap;
    ASSERT_HR(device::g_pDevice->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pHeap)));

#if defined(_DEBUG)
    wchar_t DescriptorHeapName[40];
    swprintf(DescriptorHeapName, _countof(DescriptorHeapName), L"DescriptorHeapManager(num:256) %zu", sm_descriptorHeapPool.size());
    pHeap->SetName(DescriptorHeapName);
#endif

    sm_descriptorHeapPool.emplace_back(pHeap);
    return pHeap.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapAllocator::Allocate(uint32_t Count)
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