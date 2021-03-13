#include "stdafx.h"
#include "Device.h"
#include "CommandQueueManager.h"

CommandQueueManager* CommandQueueManager::sm_pCommandQueueManager  = nullptr;
uint64_t             CommandQueueManager::sm_NumUnnamedCommandList = 0ull;

CommandQueueManager::CommandQueueManager() 
    :
    m_pDevice                (nullptr),
    m_swapChainWaitableObject(nullptr)
{
    ASSERT(sm_pCommandQueueManager == nullptr);
    sm_pCommandQueueManager = this;
}

void CommandQueueManager::CreateCommandQueueManager(ID3D12Device* pDevice)
{
    ASSERT(pDevice != nullptr);

    m_pDevice = pDevice;

    m_CommandQueue[DIRECT_TYPE] .CreateTypedQueue(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_CommandQueue[COMPUTE_TYPE].CreateTypedQueue(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
    m_CommandQueue[COPY_TYPE]   .CreateTypedQueue(pDevice, D3D12_COMMAND_LIST_TYPE_COPY);
}

// Not Used.
void CommandQueueManager::RequestCommandAllocatorList
(
    ID3D12CommandAllocator**      Allocator,
    ID3D12GraphicsCommandList**   List,
    const D3D12_COMMAND_LIST_TYPE Type
)
{
    ASSERT(CHECK_VALID_TYPE(Type), "Wrong Command List Type.");

    *Allocator = m_CommandQueue[TYPE_TO_INDEX(Type)].requestAllocator();

    ASSERT_HR(device::g_pDevice->CreateCommandList(1, Type, *Allocator, nullptr, IID_PPV_ARGS(List)));
    ASSERT_HR((*List)->Close());
}

void CommandQueueManager::RequestCommandAllocatorLists
(
    std::vector<ID3D12CommandAllocator*>& pAllocators,
    std::vector<ID3D12GraphicsCommandList*>& pLists,
    const D3D12_COMMAND_LIST_TYPE type,
    const size_t numPair
)
{
    size_t NumNeed = pAllocators.size();

    ASSERT(CHECK_VALID_TYPE(type) && (pAllocators.size() == pLists.size()) && NumNeed < numPair);
 
    if (NumNeed == numPair)
    {
        return;
    }

    m_CommandQueue[TYPE_TO_INDEX(type)].requestAllocators(pAllocators, numPair);
    pLists.reserve(numPair);

    ID3D12GraphicsCommandList* pList = nullptr;

    static uint64_t s_CommandListIndex = -1;

    for (size_t i = pLists.size(); i < numPair; ++i)
    {
        ASSERT_HR(device::g_pDevice->CreateCommandList(1, type, pAllocators[i], nullptr, IID_PPV_ARGS((ID3D12CommandList**)&pList)));
#if defined(_DEBUG)
        wchar_t CLName[40];
        swprintf(CLName, _countof(CLName), L"CommandQueueManager's CommandLists #%zu", ++s_CommandListIndex);
        pList->SetName(CLName);
#endif
        pLists.push_back(pList);
    }
}

// If CommandContext::Finish(true);
// Wait CommandQueue.executeCommandList(m_pCommandList)
void CommandQueueManager::WaitFenceValueCPUSide(uint64_t FenceValue)
{
    custom::CommandQueue& Producer = GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
    Producer.WaitFenceValueCPUSide(FenceValue);
}

void CommandQueueManager::WaitFenceValueGPUSide(uint64_t FenceValue)
{
    custom::CommandQueue& Producer = GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
    Producer.m_pCommandQueue->Wait(Producer.m_pFence, FenceValue);
}