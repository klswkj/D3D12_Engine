#pragma once

class CommandAllocatorPool
{
public:
    explicit CommandAllocatorPool()
        :
        m_commandListType(D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS),
        m_CS({})
    {
        InitializeCriticalSection(&m_CS);
    }
    ~CommandAllocatorPool() { Shutdown(); }

    inline void SetCommandType(const D3D12_COMMAND_LIST_TYPE type) { ASSERT(CHECK_VALID_TYPE(type)); m_commandListType = type; }
    ID3D12CommandAllocator* RequestAllocator(const uint64_t completedFenceValue);
    void RequestAllocators(std::vector<ID3D12CommandAllocator*>& commandAllocators, const size_t numAllocators, const uint64_t completedFenceValue);
    void DiscardAllocator(ID3D12CommandAllocator* pAllocator, const uint64_t fenceValue);
    void DiscardAllocators(std::vector<ID3D12CommandAllocator*>& commandAllocators, const uint64_t fenceValue, const size_t numAllocators);
    void DiscardAllAllocators(std::vector<ID3D12CommandAllocator*>& commandAllocators, const uint64_t fenceValue);
    inline size_t Size() const { return m_allocatorPool.size(); }

private:
    void Shutdown()
    {
        size_t AllocatorSize = m_allocatorPool.size();

        for (size_t i= 0; i < AllocatorSize; ++i)
        {
            SafeRelease(m_allocatorPool[i]);
        }

        m_allocatorPool.clear();

        if (m_CS.DebugInfo)
        {
            DeleteCriticalSection(&m_CS);
        }
    }

private:
    D3D12_COMMAND_LIST_TYPE m_commandListType;
    CRITICAL_SECTION        m_CS;

    std::vector<ID3D12CommandAllocator*>                     m_allocatorPool;
    std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_readyAllocators;
};
