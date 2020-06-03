#include "stdafx.h"
#include "CommandQueueManager.h"

CommandQueueManager::CommandQueueManager() :
    g_pDevice(nullptr),
    m_GraphicsQueue(this, D3D12_COMMAND_LIST_TYPE_DIRECT),
    m_ComputeQueue(this, D3D12_COMMAND_LIST_TYPE_COMPUTE),
    m_CopyQueue(this, D3D12_COMMAND_LIST_TYPE_COPY)
{
}

void CommandQueueManager::Create(ID3D12Device* pDevice)
{
    ASSERT(pDevice != nullptr);

    m_pDevice = pDevice;

    m_GraphicsQueue.Create(pDevice);
    m_ComputeQueue.Create(pDevice);
    m_CopyQueue.Create(pDevice);
}

void CommandQueueManager::CreateNewCommandList
(
    D3D12_COMMAND_LIST_TYPE Type,
    ID3D12GraphicsCommandList** List,
    ID3D12CommandAllocator** Allocator
)
{
    switch (Type)
    {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
        {
            *Allocator = m_GraphicsQueue.requestAllocator(); 
            break;
        }
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: 
        {
            *Allocator = m_ComputeQueue.requestAllocator(); 
            break;
        }
        case D3D12_COMMAND_LIST_TYPE_COPY: 
        {
            *Allocator = m_CopyQueue.requestAllocator(); 
            break;
        }
        default:
        {
            ASSERT(0, "Wrong Command List Type.");
        }
    }

    ASSERT_HR(m_pDevice->CreateCommandList(1, Type, *Allocator, nullptr, IID_PPV_ARGS(List)));
    SET_NAME(*List);
}

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