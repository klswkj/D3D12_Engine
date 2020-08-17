#include "stdafx.h"
#include "Device.h"
#include "CommandQueueManager.h"

// namespace-device
// ID3D12* g_pDevice;

CommandQueueManager::CommandQueueManager() :
    m_graphicsCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
    m_computeCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
    m_copyCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY)
{
}

void CommandQueueManager::Create(ID3D12Device* pDevice)
{
    ASSERT(pDevice != nullptr);

    m_graphicsCommandQueue.Create(pDevice);
    m_computeCommandQueue.Create(pDevice);
    m_copyCommandQueue.Create(pDevice);
}

void CommandQueueManager::CreateNewCommandList
(
    ID3D12GraphicsCommandList** List,
    ID3D12CommandAllocator** Allocator,
    D3D12_COMMAND_LIST_TYPE Type
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

    ASSERT_HR(device::g_pDevice->CreateCommandList(1, Type, *Allocator, nullptr, IID_PPV_ARGS(List)));
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