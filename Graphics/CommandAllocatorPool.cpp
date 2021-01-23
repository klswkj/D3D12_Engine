#include "stdafx.h"
#include "Device.h"
#include "CommandAllocatorPool.h"

// namespace-device
// ID3D12Device* g_pDevice;

CommandAllocatorPool::~CommandAllocatorPool()
{
	Shutdown();
}

ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(uint64_t CompletedFenceValue)
{
    std::lock_guard<std::mutex> LockGuard(m_allocatorMutex);

    ID3D12CommandAllocator* pAllocator = nullptr;

    if (!m_readyAllocators.empty())
    {
        std::pair<uint64_t, ID3D12CommandAllocator*>& AllocatorPair = m_readyAllocators.front();

        if (AllocatorPair.first <= CompletedFenceValue)
        {
            pAllocator = AllocatorPair.second;
            ASSERT_HR(pAllocator->Reset());
            m_readyAllocators.pop();
        }
    }

    // If no allocator's were ready to be reused, need to create a new one.
    if (pAllocator == nullptr)
    {
        ASSERT_HR(device::g_pDevice->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&pAllocator)));
#if defined(_DEBUG)
        wchar_t AllocatorName[32];
        swprintf(AllocatorName, _countof(AllocatorName), L"CommandAllocator %zu", m_allocatorPool.size());
        pAllocator->SetName(AllocatorName);
#endif
        m_allocatorPool.push_back(pAllocator);
    }

    return pAllocator;
}
void CommandAllocatorPool::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator)
{
    std::lock_guard<std::mutex> LockGuard(m_allocatorMutex);
    // That fence value indicates we are free to reset the allocator
    m_readyAllocators.push(std::make_pair(FenceValue, Allocator));
}