#include "stdafx.h"
#include "CommandAllocatorPool.h"

CommandAllocatorPool::~CommandAllocatorPool()
{
	CleanUp();
}

void CommandAllocatorPool::Create(ID3D12Device* pDevice)
{
	g_pDevice = pDevice;
}

ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(uint64_t CompletedFenceValue)
{
    // std::lock_guard<std::mutex> LockGuard(m_allocatorMutex);

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

    // If no allocator's were ready to be reused, create a new one
    if (pAllocator == nullptr)
    {
        ASSERT_HR(m_pDevice->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&pAllocator)));
        wchar_t AllocatorName[32];
        swprintf(AllocatorName, 32, L"CommandAllocator %zu", m_allocatorPool.size());
        pAllocator->SetName(AllocatorName);
        m_allocatorPool.push_back(pAllocator);
    }

    return pAllocator;
}
void CommandAllocatorPool::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator)
{
    // std::lock_guard<std::mutex> LockGuard(m_allocatorMutex);
    // That fence value indicates we are free to reset the allocator
    m_readyAllocators.push(std::make_pair(FenceValue, Allocator));
}