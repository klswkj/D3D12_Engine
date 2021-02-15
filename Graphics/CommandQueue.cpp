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
        m_FenceEventHandle(nullptr),
        m_CPUSideNextFenceValue((uint64_t)Type << 56 | 1),
        m_LastCompletedFenceValue((uint64_t)Type << 56)
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

        CloseHandle(m_FenceEventHandle);

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

        m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
        ASSERT(m_FenceEventHandle != NULL);
    }

    uint64_t CommandQueue::IncreaseCPUGPUFenceValue()
    {
        // std::lock_guard<std::mutex> LockGuard(m_fenceMutex);
        m_commandQueue->Signal(m_pFence, InterlockedGetValue(&m_CPUSideNextFenceValue));
        // return m_CPUSideNextFenceValue++;
        return InterlockedIncrement64((volatile LONG64*)&m_CPUSideNextFenceValue) - 1;
    }

    bool CommandQueue::IsFenceComplete(uint64_t FenceValue)
    {
		if (InterlockedGetValue(&m_LastCompletedFenceValue) < FenceValue)
		{
			InterlockedExchange
			(
				&m_LastCompletedFenceValue,
				(m_pFence->GetCompletedValue() < m_LastCompletedFenceValue) ?
				m_LastCompletedFenceValue : m_pFence->GetCompletedValue()
			);
		}

        return FenceValue <= m_LastCompletedFenceValue;
    }

    // GPU-Side Wait until Value.
    void CommandQueue::StallForFence(uint64_t FenceValue)
    {
        CommandQueue& Producer = device::g_commandQueueManager.GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
        m_commandQueue->Wait(Producer.m_pFence, FenceValue);
    }
    bool CommandQueue::StallForProducer(CommandQueue& Producer)
    {
        ASSERT(0 < InterlockedGetValue(&Producer.m_CPUSideNextFenceValue));
        return SUCCEEDED(m_commandQueue->Wait(Producer.m_pFence, InterlockedGetValue(&Producer.m_CPUSideNextFenceValue) - 1));
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
            // std::lock_guard<std::mutex> LockGuard(m_eventMutex);

            m_pFence->SetEventOnCompletion(FenceValue, m_FenceEventHandle);
            ::WaitForSingleObject(m_FenceEventHandle, INFINITE);
            // m_LastCompletedFenceValue = FenceValue;
            InterlockedExchange
            (
                &m_LastCompletedFenceValue, 
                (FenceValue < m_LastCompletedFenceValue)?
                m_LastCompletedFenceValue : FenceValue
            );
        }
    }

    uint64_t CommandQueue::executeCommandList(ID3D12CommandList* List)
    {
        // std::lock_guard<std::mutex> LockGuard(m_fenceMutex);

        ASSERT_HR(((ID3D12GraphicsCommandList*)List)->Close());

        // Kickoff the command list
        m_commandQueue->ExecuteCommandLists(1, &List);

		// Signal the next fence value (with the GPU)
		HRESULT hr = m_commandQueue->Signal(m_pFence, InterlockedGetValue(&m_CPUSideNextFenceValue));
        ASSERT_HR(hr);
        // LDR Present /////////////////////////////////////////////////////////////////////////////////////////////////

        // D3D12 ERROR : ID3D12CommandQueue::ExecuteCommandLists : 
        // A command list, which writes to a swapchain back buffer, 
        // may only be executed when that back buffer is the back buffer that will be presented during the next call to Present*.
        // => 스왑체인백퍼에 쓰기를 하는 commandlist는 다음 번의 "Present()"로 불려져서 다음에 화면에 보여질 백버퍼가 될 백버퍼만 execute할 수 있습니다.
        // => 다다음번의 Present() 될 backbuffer를 미리 쓰고 execute했다는건가...

        // Such a back buffer is also referred to as the "current back buffer".

        // Swap Chain : 0x000002BD5EDCB610 : 'Unnamed Object' 
        // - Current Back Buffer Buffer : 0x000002BD68C2E010 : 'g_DisplayColorBuffer 0' 
        // - Attempted Write Buffer : 0x000002BD68C2F9F0 : 
        // 'g_DisplayColorBuffer 2'[STATE_SETTING ERROR #907: EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE]
        // And increment the fence value.

        // End LRR

        // return m_CPUSideNextFenceValue++;
        return InterlockedIncrement64((volatile LONG64*)&m_CPUSideNextFenceValue) - 1;
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