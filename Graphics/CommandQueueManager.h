#pragma once
#include "stdafx.h"
#include "CommandQueue.h"
#include "GPUWorkManager.h"

#ifndef DIRECT_TYPE
#define DIRECT_TYPE 0ul
#endif

#ifndef COMPUTE_TYPE
#define COMPUTE_TYPE 1ul
#endif

#ifndef COPY_TYPE
#define COPY_TYPE 2ul
#endif

namespace custom
{
    class CommandContext;
}

class CommandQueueManager
{
	friend class custom::CommandContext;
	
public:
    CommandQueueManager();
    ~CommandQueueManager()
    {
        Shutdown();
    }

    void CreateCommandQueueManager(ID3D12Device* pDevice);

    void RequestCommandAllocatorList
    (
        ID3D12CommandAllocator** Allocator,
        ID3D12GraphicsCommandList** List,
        const D3D12_COMMAND_LIST_TYPE Type
    );

    void RequestCommandAllocatorLists
    (
        std::vector<ID3D12CommandAllocator*>& Allocators,
        std::vector<ID3D12GraphicsCommandList*>& Lists,
        const D3D12_COMMAND_LIST_TYPE Type,
        const size_t NumPair
    );
    
    void Shutdown()
    {
        m_CommandQueue[DIRECT_TYPE].Shutdown();
        m_CommandQueue[COMPUTE_TYPE].Shutdown();
        m_CommandQueue[COPY_TYPE].Shutdown();
        // m_TaskFiberManager.Shutdown();
    }

    custom::CommandQueue& GetGraphicsQueue() 
    { 
        return m_CommandQueue[DIRECT_TYPE];
    }
    custom::CommandQueue& GetComputeQueue() 
    { 
        return m_CommandQueue[COMPUTE_TYPE];
    }
    custom::CommandQueue& GetCopyQueue() 
    { 
        return m_CommandQueue[COPY_TYPE];
    }
    custom::CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        ASSERT(CHECK_VALID_TYPE(Type), "Wrong Command List Type.");

        return m_CommandQueue[TYPE_TO_INDEX(Type)];
    }
    ID3D12CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        return m_CommandQueue[TYPE_TO_INDEX(Type)].GetCommandQueue();
    }

    // Test to see if a fence has already been reached
    bool IsFenceComplete(uint64_t FenceValue)
    {
        return GetQueue(D3D12_COMMAND_LIST_TYPE(FenceValue >> 56)).IsFenceComplete(FenceValue);
    }

    // The CPU will wait for a fence to reach a specified value
    void WaitFenceValueCPUSide(uint64_t FenceValue);
    void WaitFenceValueGPUSide(uint64_t FenceValue);

    void SetSwapChainWaitableObject(HANDLE* WaitableObject)  { m_swapChainWaitableObject = WaitableObject; }

    // The CPU will wait for all command queues to empty (so that the GPU is idle)
    void WaitIdleCPUSide()
    {
        m_CommandQueue[DIRECT_TYPE].WaitCPUSide();
        m_CommandQueue[COMPUTE_TYPE].WaitCPUSide();
        m_CommandQueue[COPY_TYPE].WaitCPUSide();
    }

private:
    static CommandQueueManager* sm_pCommandQueueManager;
    static uint64_t             sm_NumUnnamedCommandList;
private:
    ID3D12Device* m_pDevice; // = device::g_pDevice;

    // 3D, Compute, Copy
    custom::CommandQueue m_CommandQueue[3];
    // custom::TaskFiberManager m_TaskFiberManager;

    HANDLE* m_swapChainWaitableObject; // = device::g_hSwapChainWaitableObject, Not used yet.
};
