#include "stdafx.h"
#include "Device.h"
#include "CommandQueueManager.h"

// namespace-device
// ID3D12* g_pDevice;

CommandQueueManager::CommandQueueManager() :
    m_graphicsCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
    m_computeCommandQueue (D3D12_COMMAND_LIST_TYPE_COMPUTE),
    m_copyCommandQueue    (D3D12_COMMAND_LIST_TYPE_COPY),
    m_pDevice(nullptr),
    m_lastSubmitFenceForSwapChain(nullptr),
    m_swapChainWaitableObject(nullptr),
    m_FenceEvent(nullptr),
    m_LastFenceValueForSwapChain(0)
{
    m_FenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT(m_FenceEvent != nullptr);
}

void CommandQueueManager::Create(ID3D12Device* pDevice)
{
    ASSERT(pDevice != nullptr);

    m_pDevice = pDevice;

    m_graphicsCommandQueue.Create(pDevice);
    m_computeCommandQueue.Create(pDevice);
    m_copyCommandQueue.Create(pDevice);
    m_TaskFiberManager.Create(pDevice);
}

void CommandQueueManager::CreateNewCommandList
(
    ID3D12GraphicsCommandList** List,
    ID3D12CommandAllocator** Allocator,
    D3D12_COMMAND_LIST_TYPE Type,
    std::wstring ID
)
{
    switch (Type)
    {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
        {
            *Allocator = m_graphicsCommandQueue.requestAllocator(); 
            break;
        }
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: 
        {
            *Allocator = m_computeCommandQueue.requestAllocator(); 
            break;
        }
        case D3D12_COMMAND_LIST_TYPE_COPY: 
        {
            *Allocator = m_copyCommandQueue.requestAllocator(); 
            break;
        }
        default:
        {
            ASSERT(0, "Wrong Command List Type.");
        }
    }

    static HRESULT createCommandListHR;

    ASSERT_HR(createCommandListHR = device::g_pDevice->CreateCommandList(1, Type, *Allocator, nullptr, IID_PPV_ARGS(List)));
    
    std::wstring  UnnamedCommandList = L"Noname CommandList ";
    static size_t NumUnnamedCommandList = 0;

    if (0 < ID.length())
    {
        ID += L"'s CommandList";
        
        (*List)->SetName(ID.c_str());
    }
    else
    {
        
        (*List)->SetName((UnnamedCommandList + std::to_wstring(NumUnnamedCommandList)).c_str());
    }
}

// If CommandContext::Finish(true);
// Wait CommandQueue.executeCommandList(m_commandList)
void CommandQueueManager::WaitForFence(uint64_t FenceValue)
{
    custom::CommandQueue& Producer = GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
    Producer.WaitForFence(FenceValue);
}

void CommandQueueManager::StallForFence(uint64_t FenceValue)
{
    custom::CommandQueue& Producer = GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
    Producer.m_commandQueue->Wait(Producer.m_pFence, FenceValue);
}

void CommandQueueManager::WaitForSwapChain()
{
    uint64_t fenceValue = m_LastFenceValueForSwapChain;

    // No fence was signaled.
    if (fenceValue == 0)
    {
        return; 
    }

    m_LastFenceValueForSwapChain = 0;
    uint64_t CompletedValue = m_lastSubmitFenceForSwapChain->GetCompletedValue();

    // It is already done.
    if (fenceValue <= CompletedValue)
    {
        return;
    }

    {
        m_lastSubmitFenceForSwapChain->SetEventOnCompletion(fenceValue, m_FenceEvent);
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }
}

void CommandQueueManager::WaitForNextFrameResources()
{
    HANDLE waitableObjects[] = { m_swapChainWaitableObject, nullptr };
    DWORD numWaitableObjects = 1;

    uint64_t fenceValue = m_LastFenceValueForSwapChain;

    if (fenceValue != 0) // means no fence was signaled
    {
        m_LastFenceValueForSwapChain = 0;
        m_lastSubmitFenceForSwapChain->SetEventOnCompletion(fenceValue, m_FenceEvent);
        waitableObjects[1] = m_FenceEvent;
        numWaitableObjects = 2;
    }

    WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);
}

void /*CommandQueueManager::*/AdvanceOrderNewCommandList(size_t NumCommandLists)
{

}
