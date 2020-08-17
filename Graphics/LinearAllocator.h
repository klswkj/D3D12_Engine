#pragma once
// Navigation : 여기에 해설 달기 IMGUI처럼 [Section]

// [Code Navigator]
// [Section] enum class LinearAllocatorType
// 
// [Section] struct     LinearBuffer
//
// [Section] class      LinearAllocationPageManager
// [Section] class      LinearAllocator

#include "LinearAllocationPage.h"

static constexpr uint32_t kGpuAllocatorPageSize = _KB(64);
static constexpr uint32_t kCpuAllocatorPageSize = _MB(2);
static constexpr uint32_t kAlignedSize = 256u;

enum class LinearAllocatorType
{
    GPU_WRITEABLE = 0, // default type. GPU - writeable(via UAV)
    CPU_WRITEABLE,      // will be upload resource.
    COUNT
};

struct LinearBuffer
{
    LinearBuffer(custom::GPUResource& BaseResource, size_t BaseOffset, size_t BaseSize)
        : buffer(BaseResource), offset(BaseOffset), size(BaseSize), pData(nullptr), GPUAddress(0)
    {
    }

    custom::GPUResource& buffer;             // The D3D buffer associated with this memory.
    void* pData;                           // The CPU-writeable address
    
    D3D12_GPU_VIRTUAL_ADDRESS GPUAddress;    // The GPU-visible address

    size_t offset;            // Offset from start of buffer resource
    size_t size;              // Reserved size of this allocation
};

class LinearAllocationPageManager
{
public:
    LinearAllocationPageManager()
    {
    }
    LinearAllocationPage* RequestPage(void);
    LinearAllocationPage* CreateNewPage(size_t PageSize = 0);

    // Discarded pages will get recycled.  This is for fixed size pages.
    void DiscardPages(uint64_t FenceID, const std::vector<LinearAllocationPage*>& Pages);

    // Freed pages will be destroyed once their fence has passed.  This is for single-use,
    // "large" pages.
    void FreeLargePages(uint64_t FenceID, const std::vector<LinearAllocationPage*>& Pages);

    void Destroy(void) 
    { 
        m_pagePool.clear(); 
    }

private:
    static LinearAllocatorType sm_autoType;

    std::vector<std::unique_ptr<LinearAllocationPage>> m_pagePool;

    std::queue<LinearAllocationPage*> m_availablePages;
    std::queue<std::pair<uint64_t, LinearAllocationPage*>> m_retiredPages;
    std::queue<std::pair<uint64_t, LinearAllocationPage*>> m_deletionQueue;

    LinearAllocatorType m_allocationType;
    // std::mutex m_Mutex;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class LinearAllocator
{
public:
    class LinearAllocationPageManager
    {
    public:
        LinearAllocationPageManager();
        LinearAllocationPage* RequestPage(void);
        LinearAllocationPage* CreateNewPage(size_t PageSize = 0ul);

        // Discarded pages will get recycled.  This is for fixed size pages.
        void DiscardPages(uint64_t FenceID, const std::vector<LinearAllocationPage*>& Pages);

        // Freed pages will be destroyed once their fence has passed.  This is for single-use,
        // "large" pages.
        void FreeLargePages(uint64_t FenceID, const std::vector<LinearAllocationPage*>& Pages);

        void Destroy(void)
        {
            m_pagePool.clear();
        }

    private:
        static LinearAllocatorType sm_autoType;

        std::vector<std::unique_ptr<LinearAllocationPage>> m_pagePool;

        std::queue<LinearAllocationPage*> m_availablePages;
        std::queue<std::pair<uint64_t, LinearAllocationPage*>> m_retiredPages;
        std::queue<std::pair<uint64_t, LinearAllocationPage*>> m_deletionQueue;

        LinearAllocatorType m_allocationType;
        // std::mutex m_Mutex;
    }; // class LinearAllocationPageManager

    LinearAllocator(LinearAllocatorType Type)
        : m_allocationType(Type), m_pageSize(0), m_currentOffset(~(size_t)0), m_currentPage(nullptr)
    {
        m_pageSize = ((Type == LinearAllocatorType::GPU_WRITEABLE) ? (kGpuAllocatorPageSize) : (kCpuAllocatorPageSize));
    }

    LinearBuffer Allocate(size_t SizeInBytes, size_t Alignment = kAlignedSize);

    void CleanupUsedPages(uint64_t FenceID);

    static void DestroyAll(void)
    {
        LinearAllocator::sm_pageManager[0].Destroy();
        LinearAllocator::sm_pageManager[1].Destroy();
    }

private:
    LinearBuffer allocateLargePage(size_t SizeInBytes);

private:
    static LinearAllocationPageManager sm_pageManager[2];
    std::vector<LinearAllocationPage*> m_retiredPages;
    std::vector<LinearAllocationPage*> m_largePageList;

    LinearAllocatorType m_allocationType;
    LinearAllocationPage* m_currentPage;
    size_t m_pageSize;
    size_t m_currentOffset;
};
