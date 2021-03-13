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
        explicit CommandQueue();
        ~CommandQueue();

        void CreateTypedQueue(ID3D12Device* const pDevice, const D3D12_COMMAND_LIST_TYPE Type);
        void Shutdown();

        uint64_t IncreaseCPUGPUFenceValue();
        bool     IsFenceComplete                 (const uint64_t fenceValue);
        void     WaitFenceValueGPUSide           (const uint64_t fenceValue);
        bool     WaitCommandQueueCompletedGPUSide(CommandQueue& customQueue);
        void     WaitFenceValueCPUSide           (const uint64_t fenceValue);

        inline void WaitCPUSide() 
        { 
            WaitFenceValueCPUSide(IncreaseCPUGPUFenceValue()); 
        }

        ID3D12CommandQueue* GetCommandQueue()   { return m_pCommandQueue; }
        ID3D12Fence*        GetFence()          { return m_pFence; }
        uint64_t            GetNextFenceValue() { return m_CPUSideNextFenceValue; }
        uint64_t volatile   GetLastCompletedFenceValue() 
        { 
            uint64_t LastCompletedFenceValue = m_pFence->GetCompletedValue();
            uint64_t LastSaveCompletedFenceValue = ::InterlockedGetValue(&m_LastCompletedFenceValue);

            ASSERT(LastSaveCompletedFenceValue <= LastCompletedFenceValue);

            if (LastSaveCompletedFenceValue < LastCompletedFenceValue)
            {
                ::InterlockedExchange(&m_LastCompletedFenceValue, m_pFence->GetCompletedValue());
                return m_LastCompletedFenceValue;
            }
            else if (LastSaveCompletedFenceValue == LastCompletedFenceValue)
            {
                return LastCompletedFenceValue;
            }

            ASSERT(false);

            return -1; // 1
        }
    private:
        uint64_t executeCommandList(ID3D12CommandList* const pList);
        uint64_t executeCommandLists(const std::vector<ID3D12GraphicsCommandList*>& pLists, const size_t startIndex, const size_t endIndex);

        ID3D12CommandAllocator* requestAllocator();
        void requestAllocators(std::vector<ID3D12CommandAllocator*>& pCommandAllocators, const size_t numAllocators);
        void discardAllocator(ID3D12CommandAllocator* pAllocator, const uint64_t fenceValue);
        void discardAllAllocators(std::vector<ID3D12CommandAllocator*>& pCommandAllocators, const uint64_t fenceValue);
        void discardAllocators(std::vector<ID3D12CommandAllocator*>& pCommandAllocators, const uint64_t fenceValue, const size_t numAllocators);

    private:
        D3D12_COMMAND_LIST_TYPE m_type;
        CommandAllocatorPool    m_allocatorPool;
        ID3D12CommandQueue*     m_pCommandQueue;

        // Lifetime of these objects is managed by the descriptor cache
        ID3D12Fence*      m_pFence;
        uint64_t volatile m_CPUSideNextFenceValue; // CPU-Side Currently will be recorded value.
        uint64_t volatile m_LastCompletedFenceValue;
        HANDLE            m_FenceEventHandle;
        CRITICAL_SECTION  m_CS;
	};
}