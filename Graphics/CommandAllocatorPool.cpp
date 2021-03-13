#include "stdafx.h"
#include "Device.h"
#include "CommandAllocatorPool.h"

ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(const uint64_t completedFenceValue)
{
    EnterCriticalSection(&m_CS);

    ID3D12CommandAllocator* pAllocator = nullptr;

    if (!m_readyAllocators.empty())
    {
        std::pair<uint64_t, ID3D12CommandAllocator*>& AllocatorPair = m_readyAllocators.front();

        if (AllocatorPair.first <= completedFenceValue)
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

    LeaveCriticalSection(&m_CS);

    return pAllocator;
}

void CommandAllocatorPool::RequestAllocators
(
    std::vector<ID3D12CommandAllocator*>& commandAllocators,
    const size_t numAllocators, const uint64_t completedFenceValue
)
{
    EnterCriticalSection(&m_CS);

    commandAllocators.reserve(numAllocators + commandAllocators.size());
    
	while (!m_readyAllocators.empty() && (commandAllocators.size() < numAllocators))
	{
		std::pair<uint64_t, ID3D12CommandAllocator*>& AllocatorPair = m_readyAllocators.front();

		if (AllocatorPair.first <= completedFenceValue)
		{
			commandAllocators.push_back(AllocatorPair.second);
			ASSERT_HR(AllocatorPair.second->Reset());
			m_readyAllocators.pop();
		}
		else
		{
			break;
		}
	}

    ID3D12CommandAllocator* pAllocator = nullptr;

    while ((commandAllocators.size() < numAllocators))
    {
        ASSERT_HR(device::g_pDevice->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&pAllocator)));
#if defined(_DEBUG)
        wchar_t AllocatorName[32];
        swprintf(AllocatorName, _countof(AllocatorName), L"CommandAllocator %zu", m_allocatorPool.size());
        pAllocator->SetName(AllocatorName);
#endif
        m_allocatorPool.push_back(pAllocator);
        commandAllocators.push_back(pAllocator);
    }

    LeaveCriticalSection(&m_CS);

    ID3D12Device* pDevice1 = nullptr;
    ID3D12Device* pDevice2;

    commandAllocators.front()->GetDevice(IID_PPV_ARGS(&pDevice1));
    commandAllocators.back()->GetDevice(IID_PPV_ARGS(&pDevice2));

    ASSERT(pDevice1 == pDevice2);

    SafeRelease(pDevice1);
    SafeRelease(pDevice2);
}

void CommandAllocatorPool::DiscardAllocator(ID3D12CommandAllocator* pAllocator, const uint64_t fenceValue)
{
    EnterCriticalSection(&m_CS);
    m_readyAllocators.push({ fenceValue, pAllocator });
    LeaveCriticalSection(&m_CS);
}

void CommandAllocatorPool::DiscardAllocators
(
    std::vector<ID3D12CommandAllocator*>& pCommandAllocators,
    const uint64_t fenceValue,
    const size_t numCommandAllocators
)
{
    size_t Size = pCommandAllocators.size();

    ASSERT(numCommandAllocators <= Size);

    EnterCriticalSection(&m_CS);

    for (size_t i = Size - numCommandAllocators; i < Size; ++i)
    {
        m_readyAllocators.push({ fenceValue, pCommandAllocators[i] });
    }

    LeaveCriticalSection(&m_CS);

    pCommandAllocators.erase(pCommandAllocators.end() - numCommandAllocators, pCommandAllocators.end());
    pCommandAllocators.shrink_to_fit();
}

void CommandAllocatorPool::DiscardAllAllocators
(
    std::vector<ID3D12CommandAllocator*>& pCommandAllocators,
    const uint64_t fenceValue
)
{
    size_t Size = pCommandAllocators.size();

    EnterCriticalSection(&m_CS);

    for (auto& e : pCommandAllocators)
    {
        m_readyAllocators.push({ fenceValue, e });
    }

    LeaveCriticalSection(&m_CS);
}