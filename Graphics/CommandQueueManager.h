#pragma once
#include "stdafx.h"
#include "CommandQueue.h"

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

    void Create(ID3D12Device* pDevice);

    void CreateNewCommandList
    (
        ID3D12GraphicsCommandList** List,
        ID3D12CommandAllocator** Allocator,
        D3D12_COMMAND_LIST_TYPE Type
    );

    void Shutdown()
    {
        m_graphicsCommandQueue.Shutdown();
        m_computeCommandQueue.Shutdown();
        m_copyCommandQueue.Shutdown();
    }

    custom::CommandQueue& GetGraphicsQueue() 
    { 
        return m_graphicsCommandQueue;
    }
    custom::CommandQueue& GetComputeQueue() 
    { 
        return m_computeCommandQueue; 
    }
    custom::CommandQueue& GetCopyQueue() 
    { 
        return m_copyCommandQueue; 
    }
    custom::CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        switch (Type)
        {
            case D3D12_COMMAND_LIST_TYPE_DIRECT:
            {
                return m_graphicsCommandQueue;
            }
            case D3D12_COMMAND_LIST_TYPE_COMPUTE: 
            {
                return m_computeCommandQueue; 
            }
            case D3D12_COMMAND_LIST_TYPE_COPY: 
            {
                return m_copyCommandQueue; 
            }
            default:
            {
                ASSERT(0, "Wrong Command List Type.");
                return m_graphicsCommandQueue;
            }
        }
    }

    ID3D12CommandQueue* GetCommandQueue()
    {
        return m_graphicsCommandQueue.GetCommandQueue();
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
        m_graphicsCommandQueue.WaitForIdle();
        m_computeCommandQueue.WaitForIdle();
        m_copyCommandQueue.WaitForIdle();
    }

private:
    ID3D12Device* m_pDevice;

    custom::CommandQueue m_graphicsCommandQueue;
    custom::CommandQueue m_computeCommandQueue;
    custom::CommandQueue m_copyCommandQueue;
};
