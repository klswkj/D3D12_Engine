#include "stdafx.h"
#include "CommandQueue.h"
#include "CommandQueueManager.h"
#include "Device.h"

namespace custom
{
    CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE Type)
        : 
        m_type(Type), m_allocatorPool(Type),
        m_commandQueue(nullptr), m_pFence(nullptr), 
        m_fenceEventHandle(nullptr),
        m_nextFenceValue((uint64_t)Type << 56 | 1),
        m_lastCompletedFenceValue((uint64_t)Type << 56)
    {
    }
    CommandQueue::~CommandQueue()
    {
        Shutdown();
    }

    void CommandQueue::Shutdown()
    {
		if (m_commandQueue == nullptr)
		{
			return;
		}

        m_allocatorPool.~CommandAllocatorPool();

        CloseHandle(m_fenceEventHandle);

        SafeRelease(m_pFence);
        SafeRelease(m_commandQueue);
    }

    void CommandQueue::Create(ID3D12Device* pDevice)
    {
        ASSERT(pDevice != nullptr);
        ASSERT(m_commandQueue == nullptr);
        ASSERT(m_allocatorPool.Size() == 0);

        D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
        ZeroMemory(&commandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
        commandQueueDesc.Type = m_type;
        commandQueueDesc.NodeMask = 1;
        ASSERT_HR(pDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_commandQueue)));

        static size_t NumCommandQueue = -1;

        ASSERT_HR(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));

#ifdef _DEBUG
        m_pFence->SetName(StringToWString("CommandQueue::m_pFence" + std::to_string(++NumCommandQueue)).c_str());
        m_commandQueue->SetName(StringToWString("CommandQueue::m_CommandQueue" + std::to_string(++NumCommandQueue)).c_str());
#endif

        m_pFence->Signal((uint64_t)m_type << 56);

        m_fenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
        ASSERT(m_fenceEventHandle != NULL);
    }

    uint64_t CommandQueue::IncrementFence(void)
    {
        std::lock_guard<std::mutex> LockGuard(m_fenceMutex);
        m_commandQueue->Signal(m_pFence, m_nextFenceValue);
        return m_nextFenceValue++;
    }
    bool CommandQueue::IsFenceComplete(uint64_t FenceValue)
    {
		if (m_lastCompletedFenceValue < FenceValue)
		{
            m_lastCompletedFenceValue = (m_pFence->GetCompletedValue() < m_lastCompletedFenceValue) ?
                m_lastCompletedFenceValue : m_pFence->GetCompletedValue();
		}
        return FenceValue <= m_lastCompletedFenceValue;
    }

    // TODO 3 : 혹시모를 Swapchain starvation대비해서
    // FrameContext(Swapchain 갯수만큼), WaitForNextFrameResources 만들기
    // 그래서 CommandContext.Finish(bool WaitForCompletion = false, bool WaitForSwapChain = false)형식으로 해서
    
    void CommandQueue::StallForFence(uint64_t FenceValue)
    {
        CommandQueue& Producer = device::g_commandQueueManager.GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
        m_commandQueue->Wait(Producer.m_pFence, FenceValue);
    }
    bool CommandQueue::StallForProducer(CommandQueue& Producer)
    {
        ASSERT(0 < Producer.m_nextFenceValue);
        return SUCCEEDED(m_commandQueue->Wait(Producer.m_pFence, Producer.m_nextFenceValue - 1));
        
    }
    void CommandQueue::WaitForFence(uint64_t FenceValue)
    {
        if (IsFenceComplete(FenceValue))
        {
            return;
        }
        // TODO:  Think about how this might affect a multi-threaded situation.  
        // Suppose thread A wants to wait for fence 100, then thread B comes along and wants to wait for 99.  
        // If the fence can only have one event set on completion, 
        // then thread B has to wait for 100 before it knows 99 is ready.  
        // Maybe insert sequential events?
        {
            std::lock_guard<std::mutex> LockGuard(m_eventMutex);

            m_pFence->SetEventOnCompletion(FenceValue, m_fenceEventHandle);
            WaitForSingleObject(m_fenceEventHandle, INFINITE);
            m_lastCompletedFenceValue = FenceValue;
        }
    }

    uint64_t CommandQueue::executeCommandList(ID3D12CommandList* List)
    {
        std::lock_guard<std::mutex> LockGuard(m_fenceMutex);

        ASSERT_HR(((ID3D12GraphicsCommandList*)List)->Close());

        // Kickoff the command list
        m_commandQueue->ExecuteCommandLists(1, &List);

		// Signal the next fence value (with the GPU)
		HRESULT hr = m_commandQueue->Signal(m_pFence, m_nextFenceValue);

        // LDR Present /////////////////////////////////////////////////////////////////////////////////////////////////

        // D3D12 ERROR : ID3D12CommandQueue::ExecuteCommandLists : 
        // A command list, which writes to a swapchain back buffer, 
        // may only be executed when that back buffer is the back buffer that will be presented during the next call to Present*.
        // Such a back buffer is also referred to as the "current back buffer".

        // Swap Chain : 0x000002BD5EDCB610 : 'Unnamed Object' 
        // - Current Back Buffer Buffer : 0x000002BD68C2E010 : 'g_DisplayColorBuffer 0' 
        // - Attempted Write Buffer : 0x000002BD68C2F9F0 : 
        // 'g_DisplayColorBuffer 2'[STATE_SETTING ERROR #907: EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE]
        // And increment the fence value.

        // End LRR

        return m_nextFenceValue++;
    }

    ID3D12CommandAllocator* CommandQueue::requestAllocator()
    {
        uint64_t CompletedFence = m_pFence->GetCompletedValue();

        return m_allocatorPool.RequestAllocator(CompletedFence);
    }
    void CommandQueue::discardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator)
    {
        m_allocatorPool.DiscardAllocator(FenceValue, Allocator);
    }
}