#pragma once
#include "stdafx.h"
#include "CommandQueue.h"

class CommandQueueManager
{
    friend class CommandContext;
public:
    CommandQueueManager();
    ~CommandQueueManager()
    {
        CleanUp();
    }

    void Create(ID3D12Device* pDevice);

    void CreateNewCommandList
    (
        D3D12_COMMAND_LIST_TYPE Type,
        ID3D12GraphicsCommandList** List,
        ID3D12CommandAllocator** Allocator
    );

    void CleanUp()
    {
        m_GraphicsQueue.CleanUp();
        m_ComputeQueue.CleanUp();
        m_CopyQueue.CleanUp();
    }

    custom::CommandQueue& GetGraphicsQueue(void) 
    { 
        return m_GraphicsQueue; 
    }
    custom::CommandQueue& GetComputeQueue(void) 
    { 
        return m_ComputeQueue; 
    }
    custom::CommandQueue& GetCopyQueue(void) 
    { 
        return m_CopyQueue; 
    }
    custom::CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        switch (Type)
        {
            case D3D12_COMMAND_LIST_TYPE_DIRECT:
            {
                return m_GraphicsQueue;
            }
            case D3D12_COMMAND_LIST_TYPE_COMPUTE: 
            {
                return m_ComputeQueue; 
            }
            case D3D12_COMMAND_LIST_TYPE_COPY: 
            {
                return m_CopyQueue; 
            }
            default:
            {
                ASSERT(0, "Wrong Command List Type.");
            }
        }
    }
    ID3D12CommandQueue* GetCommandQueue()
    {
        return m_GraphicsQueue.GetCommandQueue();
    }

    // Test to see if a fence has already been reached
    bool IsFenceComplete(uint64_t FenceValue)
    {
        return GetQueue(D3D12_COMMAND_LIST_TYPE(FenceValue >> 56)).IsFenceComplete(FenceValue);
    }

    // The CPU will wait for a fence to reach a specified value
    void WaitForFence(uint64_t FenceValue);

    void StallForFence(uint64_t FenceValue);

    // The CPU will wait for all command queues to empty (so that the GPU is idle)
    void IdleGPU(void)
    {
        m_GraphicsQueue.WaitForIdle();
        m_ComputeQueue.WaitForIdle();
        m_CopyQueue.WaitForIdle();
    }

private:
    custom::CommandQueue m_GraphicsQueue;
    custom::CommandQueue m_ComputeQueue;
    custom::CommandQueue m_CopyQueue;
};
