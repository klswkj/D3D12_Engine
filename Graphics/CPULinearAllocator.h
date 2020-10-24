#pragma once

class CPULinearAllocationPage
{
public:
    CPULinearAllocationPage(void* pAddress, size_t DataSize)
        : m_CPUAddress(pAddress), m_Size(DataSize)
    {
    }

    ~CPULinearAllocationPage()
    {
        if (m_CPUAddress)
        {
            free(m_CPUAddress);
        }
    }

    void* m_CPUAddress;
    size_t m_Size;
};

struct CPULinearBuffer
{
    CPULinearBuffer() = default;

    CPULinearBuffer(CPULinearAllocationPage* Page, size_t BaseOffset, size_t BaseSize)
        : pPage(Page), pData(nullptr), offset(BaseOffset), size(BaseSize)
    {
    }

    CPULinearBuffer& operator=(const CPULinearBuffer& rhs)
    {
        pPage = rhs.pPage;
        pData = rhs.pData;
        offset = rhs.offset;
        size = rhs.size;
    }

    CPULinearAllocationPage* pPage;
    void* pData;
    size_t offset;            // Offset from start of buffer resource(CPU-LinearAllocationPage)
    size_t size;              // Reserved size of this allocation
};

class CPULinearAllocator
{
public:
    class CPULinearAllocationPageManager
    {
    public:
        CPULinearAllocationPageManager() = default;
        CPULinearAllocationPage* RequestPage();
        CPULinearAllocationPage* CreateNewPage(size_t PageSize = 0ul);

        // Discarded pages will get recycled.  This is for fixed size pages.
        void DiscardPages(const std::vector<CPULinearAllocationPage*>& Pages);

        // Freed pages will be destroyed once their fence has passed.  This is for single-use,
        // "large" pages.
        void FreeLargePages(const std::vector<CPULinearAllocationPage*>& Pages);

        void Destroy()
        {
            m_pagePool.clear();
        }

    private:
        std::vector<std::unique_ptr<CPULinearAllocationPage>> m_pagePool;

        std::queue<CPULinearAllocationPage*> m_availablePages;
        std::queue<CPULinearAllocationPage*> m_retiredPages;
        std::queue<CPULinearAllocationPage*> m_deletionQueue;

        std::mutex m_Mutex;
    }; // class LinearAllocationPageManager

    CPULinearAllocator()
        : m_pageSize(0), m_currentOffset(~(size_t)0), m_currentPage(nullptr)
    {
        m_pageSize = 2097152ul;// _MB(2);
    }

    CPULinearBuffer Allocate(size_t SizeInBytes, size_t Alignment = 256ul); // TODO : Alignment ªÁ¿Ã¡Ó ¡ˆƒ—∫¡æﬂµ 

    void CleanupUsedPages();

    static void DestroyAll()
    {
        CPULinearAllocator::sm_pageManager.Destroy();
    }

private:
    CPULinearBuffer allocateLargePage(size_t SizeInBytes);

private:
    static CPULinearAllocationPageManager sm_pageManager;
    std::vector<CPULinearAllocationPage*> m_retiredPages;
    std::vector<CPULinearAllocationPage*> m_largePageList;

    CPULinearAllocationPage* m_currentPage;
    size_t m_pageSize;
    size_t m_currentOffset;
};
