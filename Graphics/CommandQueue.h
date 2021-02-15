#pragma once
#include "stdafx.h"
#include "CommandAllocatorPool.h"

class CommandQueueManager;

namespace custom
{
    class CommandContext;

	class CommandQueue
	{
        friend class CommandQueueManager;
        friend class CommandContext;
    public:
        CommandQueue(D3D12_COMMAND_LIST_TYPE Type);
        ~CommandQueue();

        void Create(ID3D12Device* pDevice);
        void Shutdown();

        uint64_t IncreaseCPUGPUFenceValue();
        bool IsFenceComplete(uint64_t FenceValue);
        void StallForFence(uint64_t FenceValue);
        bool StallForProducer(CommandQueue& Producer);
        void WaitForFence(uint64_t FenceValue);

        void WaitForIdle(void) 
        { 
            WaitForFence(IncreaseCPUGPUFenceValue()); 
        }

        ID3D12CommandQueue* GetCommandQueue()   { return m_commandQueue; }
        ID3D12Fence*        GetFence()          { return m_pFence; }
        uint64_t            GetNextFenceValue() { return m_CPUSideNextFenceValue; }

    private:
        uint64_t executeCommandList(ID3D12CommandList* List);
        ID3D12CommandAllocator* requestAllocator();
        void discardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator* Allocator);

    private:
        ID3D12CommandQueue* m_commandQueue;

        const D3D12_COMMAND_LIST_TYPE m_type;

        CommandAllocatorPool m_allocatorPool;

        // Lifetime of these objects is managed by the descriptor cache
        ID3D12Fence*      m_pFence;
        uint64_t volatile m_CPUSideNextFenceValue; // CPU-Side Currently will be recorded value.
        uint64_t volatile m_LastCompletedFenceValue;
        HANDLE            m_FenceEventHandle;

        // std::mutex m_fenceMutex;
        // std::mutex m_eventMutex;
	};
}