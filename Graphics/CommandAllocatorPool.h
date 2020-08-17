#pragma once

class CommandAllocatorPool
{
public:
    CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type)
        : m_commandListType(Type)
    {
    }
    ~CommandAllocatorPool();

    ID3D12CommandAllocator* RequestAllocator(uint64_t CompletedFenceValue);
    void DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator);

    inline size_t Size() const
    { 
        return m_allocatorPool.size(); 
    }

private:
    void Shutdown()
    {
        for (size_t i{ 0 }; i < m_allocatorPool.size(); ++i)
        {
            SafeRelease(m_allocatorPool[i]);
        }
        m_allocatorPool.clear();
    }

private:
    const D3D12_COMMAND_LIST_TYPE m_commandListType;

    std::vector<ID3D12CommandAllocator*> m_allocatorPool;
    std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_readyAllocators;
    // std::mutex m_allocatorMutex;
};
