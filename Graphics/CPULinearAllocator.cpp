#include "stdafx.h"
#include "CPULinearAllocator.h"

#pragma region LINEAR_ALLOCATION_PAGE_MANAGER

CPULinearAllocator::CPULinearAllocationPageManager CPULinearAllocator::sm_pageManager;

CPULinearAllocationPage* CPULinearAllocator::CPULinearAllocationPageManager::RequestPage()
{
    std::lock_guard<std::mutex> LockGuard(m_Mutex);

    while (!m_retiredPages.empty())
    {
        m_availablePages.push(m_retiredPages.front());
        m_retiredPages.pop();
    }

    CPULinearAllocationPage* PagePtr = nullptr;

    if (!m_availablePages.empty())
    {
        PagePtr = m_availablePages.front();
        m_availablePages.pop();
    }
    else
    {
        PagePtr = CreateNewPage(4096);
        m_pagePool.emplace_back(PagePtr);
    }

    return PagePtr;
}

CPULinearAllocationPage* CPULinearAllocator::CPULinearAllocationPageManager::CreateNewPage(size_t PageSize /* = 4096*/)
{
    void* pBuffer = malloc(PageSize);

    return new CPULinearAllocationPage(pBuffer, PageSize);
}

void CPULinearAllocator::CPULinearAllocationPageManager::DiscardPages(const std::vector<CPULinearAllocationPage*>& UsedPages)
{
    std::lock_guard<std::mutex> LockGuard(m_Mutex);

    for (auto& iter : UsedPages)
    {
        m_retiredPages.push(iter);
    }
}

void CPULinearAllocator::CPULinearAllocationPageManager::FreeLargePages(const std::vector<CPULinearAllocationPage*>& LargePages)
{
    std::lock_guard<std::mutex> LockGuard(m_Mutex);

    while (!m_deletionQueue.empty())
    {
        delete m_deletionQueue.front();
        m_deletionQueue.pop();
    }

    for (auto& iter : LargePages)
    {
        // (*iter)->SafeUnmap();
        m_deletionQueue.push(iter);
    }
}

#pragma endregion LINEAR_ALLOCATION_PAGE_MANAGER

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

CPULinearBuffer CPULinearAllocator::allocateLargePage(size_t SizeInBytes)
{
    CPULinearAllocationPage* OneOff = sm_pageManager.CreateNewPage(SizeInBytes);
    m_largePageList.push_back(OneOff);

    CPULinearBuffer ret(OneOff, 0, SizeInBytes);

    return ret;
}

void CPULinearAllocator::CleanupUsedPages()
{
    if (m_currentPage == nullptr)
    {
        return;
    }

    m_retiredPages.push_back(m_currentPage);
    m_currentPage = nullptr;
    m_currentOffset = 0;

    sm_pageManager.DiscardPages(m_retiredPages);
    m_retiredPages.clear();

    sm_pageManager.FreeLargePages(m_largePageList);
    m_largePageList.clear();
}

// 여기가 Scoped_Ptr이 불러오는 최초의 함수.
CPULinearBuffer CPULinearAllocator::Allocate(size_t SizeInBytes, size_t Alignment)
{
    const size_t AlignmentMask = Alignment - 1;

    // Assert that it's a power of two.
    ASSERT((AlignmentMask & Alignment) == 0);

    // Align the allocation
    const size_t AlignedSize = HashInternal::AlignUpWithMask(SizeInBytes, AlignmentMask);

    if (m_pageSize < AlignedSize)
    {
        return allocateLargePage(AlignedSize);
    }
    m_currentOffset = HashInternal::AlignUp(m_currentOffset, Alignment);

    if (m_pageSize < m_currentOffset + AlignedSize)
    {
        ASSERT(m_currentPage != nullptr);
        m_retiredPages.push_back(m_currentPage);
        m_currentPage = nullptr;
    }

    if (m_currentPage == nullptr)
    {
        m_currentPage = sm_pageManager.RequestPage();
        m_currentOffset = 0;
    }

    CPULinearBuffer ret(m_currentPage, m_currentOffset, AlignedSize);
    ret.pData = (uint8_t*)m_currentPage->m_CPUAddress + m_currentOffset;

    m_currentOffset += AlignedSize;

    return ret;
}
