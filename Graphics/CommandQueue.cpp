#include "stdafx.h"
#include "CommandQueue.h"
#include "CommandQueueManager.h"
#include "Device.h"
#include "CustomFence.h"
#include "d3dx12Residency.h"
namespace custom
{
    CommandQueue::CommandQueue()
        :
        m_type(D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS),
        m_pFence          (nullptr),
        m_pCommandQueue   (nullptr),
        m_FenceEventHandle(nullptr),
        m_CS({}),
        m_CPUSideNextFenceValue  (0ull),
        m_LastCompletedFenceValue(0ull)
    {
        // CustomFence::SetFence(m_pFence, Type);
        if (!m_CS.DebugInfo)
        {
            ::InitializeCriticalSection(&m_CS);
        }
    }
    CommandQueue::~CommandQueue()
    {
        Shutdown();
    }

    void CommandQueue::Shutdown()
    {
		if (m_pCommandQueue == nullptr)
		{
			return;
		}

        m_allocatorPool.~CommandAllocatorPool();

        CloseHandle(m_FenceEventHandle);

        SafeRelease(m_pFence);
        SafeRelease(m_pCommandQueue);

        if (m_CS.DebugInfo)
        {
            ::DeleteCriticalSection(&m_CS);
        }
    }

    void CommandQueue::CreateTypedQueue(ID3D12Device* const pDevice, const D3D12_COMMAND_LIST_TYPE type)
    {
        ASSERT(pDevice != nullptr);
        ASSERT(m_pCommandQueue == nullptr);
        ASSERT(m_allocatorPool.Size() == 0);

        m_type                    = type;
        m_CPUSideNextFenceValue   = (uint64_t)type << 56 | 1;
        m_LastCompletedFenceValue = (uint64_t)type << 56;
        m_allocatorPool.SetCommandType(type);

        D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
        ZeroMemory(&commandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
        commandQueueDesc.Type = m_type;
        commandQueueDesc.NodeMask = 1;
        ASSERT_HR(pDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_pCommandQueue)));

        ASSERT_HR(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));

#ifdef _DEBUG
        static size_t NumCommandQueue = -1;
        m_pFence->SetName(StringToWString("CommandQueue::m_pFence" + std::to_string(++NumCommandQueue)).c_str());
        m_pCommandQueue->SetName(StringToWString("CommandQueue::m_CommandQueue" + std::to_string(NumCommandQueue)).c_str());
#endif
        m_pFence->Signal(((UINT64)(uint64_t)m_type << 56));

        m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
        ASSERT(m_FenceEventHandle != NULL);
    }

    uint64_t CommandQueue::IncreaseCPUGPUFenceValue()
    {
        // std::lock_guard<std::mutex> LockGuard(m_fenceMutex);
        HRESULT HardwareResult = S_OK;
        // Signal the next fence value (with the GPU)
        uint64_t ContextFenceValue = InterlockedIncrement64((volatile LONG64*)&m_CPUSideNextFenceValue) - 1;
        ASSERT_HR(HardwareResult = m_pCommandQueue->Signal(m_pFence, ContextFenceValue));

        // return m_CPUSideNextFenceValue++;
        return ContextFenceValue;
    }

    bool CommandQueue::IsFenceComplete(const uint64_t fenceValue)
    {
        ASSERT(CHECK_VALID_FENCE_VALUE(m_type, fenceValue));

		if (InterlockedGetValue(&m_LastCompletedFenceValue) < fenceValue)
		{
            uint64_t LastCompletedFenceValue = m_pFence->GetCompletedValue();

			InterlockedExchange
			(
				&m_LastCompletedFenceValue,
				(LastCompletedFenceValue < m_LastCompletedFenceValue) ?
				m_LastCompletedFenceValue : LastCompletedFenceValue
			);
		}

        return fenceValue <= InterlockedGetValue(&m_LastCompletedFenceValue);
    }

    // GPU-Side Wait until Value.
    void CommandQueue::WaitFenceValueGPUSide(const uint64_t fenceValue)
    {
        CommandQueue& Producer = device::g_commandQueueManager.GetQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56));
        m_pCommandQueue->Wait(Producer.m_pFence, fenceValue);
    }
    bool CommandQueue::WaitCommandQueueCompletedGPUSide(CommandQueue& customQueue)
    {
        uint64_t CPUSideNextFenceValue = InterlockedGetValue(&customQueue.m_CPUSideNextFenceValue);
        ASSERT(CHECK_VALID_FENCE_VALUE(customQueue.m_type, CPUSideNextFenceValue));
        return SUCCEEDED(m_pCommandQueue->Wait(customQueue.m_pFence, CPUSideNextFenceValue - 1));
    }
    void CommandQueue::WaitFenceValueCPUSide(const uint64_t fenceValue)
    {
        ASSERT(CHECK_VALID_FENCE_VALUE(m_type, fenceValue));

        if (IsFenceComplete(fenceValue))
        {
            return;
        }

        {
            m_pFence->SetEventOnCompletion(fenceValue, m_FenceEventHandle);
            ::WaitForSingleObject(m_FenceEventHandle, INFINITE);
            // m_LastCompletedFenceValue = FenceValue;
            InterlockedExchange
            (
                &m_LastCompletedFenceValue, 
                (fenceValue < m_LastCompletedFenceValue)?
                m_LastCompletedFenceValue : fenceValue
            );
        }
    }

    uint64_t CommandQueue::executeCommandList(ID3D12CommandList* const pList)
    {
        // std::lock_guard<std::mutex> LockGuard(m_fenceMutex);

        ASSERT_HR(((ID3D12GraphicsCommandList*)pList)->Close());

        // Kickoff the command list
        m_pCommandQueue->ExecuteCommandLists(1, &pList);

        HRESULT HardwareResult = S_OK;
        // Signal the next fence value (with the GPU)
        uint64_t ContextFenceValue = InterlockedIncrement64((volatile LONG64*)&m_CPUSideNextFenceValue) - 1;
        ASSERT_HR(HardwareResult = m_pCommandQueue->Signal(m_pFence, ContextFenceValue));

        // return m_CPUSideNextFenceValue++;
        return ContextFenceValue;
    }
    uint64_t CommandQueue::executeCommandLists
    (
        const std::vector<ID3D12GraphicsCommandList*>& Lists, 
        const size_t startIndex, const size_t endIndex
    )
    {
        ASSERT(false);

        size_t Size = Lists.size();

        ASSERT((startIndex <= endIndex) && (endIndex < Size));

        for (size_t i = startIndex; i <= endIndex; ++i)
        {
           ASSERT_HR(Lists[i]->Close());
        }

        HRESULT HardwareResult = S_OK;

        ::EnterCriticalSection(&m_CS);

        // Execute CommandLists
        m_pCommandQueue->ExecuteCommandLists((UINT)(endIndex - startIndex + 1), (ID3D12CommandList* const*)(Lists.data() + startIndex));
        
        // Signal the next fence value (with the GPU)
        uint64_t ContextFenceValue = InterlockedIncrement64((volatile LONG64*)&m_CPUSideNextFenceValue) - 1;
        ASSERT_HR(HardwareResult = m_pCommandQueue->Signal(m_pFence, ContextFenceValue));

        ::LeaveCriticalSection(&m_CS);

        return ContextFenceValue;
    }

    ID3D12CommandAllocator* CommandQueue::requestAllocator()
    {
        uint64_t CompletedFence = m_pFence->GetCompletedValue();

        return m_allocatorPool.RequestAllocator(CompletedFence);
    }
    void CommandQueue::requestAllocators(std::vector<ID3D12CommandAllocator*>& commandAllocators, const size_t numAllocators)
    {
        uint64_t CompletedFence = m_pFence->GetCompletedValue();
        return m_allocatorPool.RequestAllocators(commandAllocators, numAllocators, CompletedFence);
    }

    void CommandQueue::discardAllocator(ID3D12CommandAllocator* pAllocator, const uint64_t fenceValue)
    {
        m_allocatorPool.DiscardAllocator(pAllocator, fenceValue);
    }
    void CommandQueue::discardAllAllocators
    (
        std::vector<ID3D12CommandAllocator*>& pCommandAllocators, 
        const uint64_t fenceValue
    )
    {
        m_allocatorPool.DiscardAllAllocators(pCommandAllocators, fenceValue);
    }
    void CommandQueue::discardAllocators
    (
        std::vector<ID3D12CommandAllocator*>& pCommandAllocators,
        const uint64_t fenceValue, const size_t numAllocators
    )
    {
        m_allocatorPool.DiscardAllocators(pCommandAllocators, fenceValue, numAllocators);
    }
}