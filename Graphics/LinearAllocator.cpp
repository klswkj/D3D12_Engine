#include "stdafx.h"
#include "LinearAllocator.h"

LinearAllocationPage* LinearAllocatorPageManager::RequestPage()
{
    // lock_guard<mutex> LockGuard(m_Mutex);

    while (!m_retiredPages.empty() && m_commandQueueManager.IsFenceComplete(m_retiredPages.front().first))
    {
        m_availablePages.push(m_retiredPages.front().second);
        m_retiredPages.pop();
    }

    LinearAllocationPage* PagePtr = nullptr;

    if (!m_availablePages.empty())
    {
        PagePtr = m_availablePages.front();
        m_availablePages.pop();
    }
    else
    {
        PagePtr = CreateNewPage();
        m_pagePool.emplace_back(PagePtr);
    }

    return PagePtr;
}

void LinearAllocatorPageManager::DiscardPages(uint64_t FenceValue, const std::vector<LinearAllocationPage*>& UsedPages)
{
	// lock_guard<mutex> LockGuard(m_Mutex);
	for (auto iter = UsedPages.begin(); iter != UsedPages.end(); ++iter)
	{
        m_retiredPages.push({ FenceValue, *iter });
	}
}

void LinearAllocatorPageManager::FreeLargePages(uint64_t FenceValue, const std::vector<LinearAllocationPage*>& LargePages)
{
    // lock_guard<mutex> LockGuard(m_Mutex);

    while (!m_deletionQueue.empty() && m_commandQueueManager.IsFenceComplete(m_deletionQueue.front().first))
    {
        delete m_deletionQueue.front().second;
        m_deletionQueue.pop();
    }

    for (auto iter = LargePages.begin(); iter != LargePages.end(); ++iter)
    {
        (*iter)->SafeUnmap();
        m_deletionQueue.push({ FenceValue, *iter });
    }
}

LinearAllocationPage* LinearAllocatorPageManager::CreateNewPage(size_t PageSize)
{
    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC ResourceDesc;
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Alignment = 0;
    ResourceDesc.Height = 1;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_RESOURCE_STATES DefaultUsage;

    if (m_allocationType == LinearAllocatorType::GPU_WRITEABLE)
    {
        HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        ResourceDesc.Width = PageSize == 0 ? kGpuAllocatorPageSize : PageSize;
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        DefaultUsage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    else
    {
        HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        ResourceDesc.Width = PageSize == 0 ? kCpuAllocatorPageSize : PageSize;
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        DefaultUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    ID3D12Resource* pBuffer;
    ASSERT_HR
    (
		m_pDevice->CreateCommittedResource
		(
			&HeapProps, D3D12_HEAP_FLAG_NONE,
			&ResourceDesc, DefaultUsage, nullptr, IID_PPV_ARGS(&pBuffer)
        )
    );

    SET_NAME(pBuffer);

    return new LinearAllocationPage(pBuffer, DefaultUsage);
}

///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////


void LinearAllocator::CleanupUsedPages(uint64_t FenceID)
{
    if (m_currentPage == nullptr)
        return;

    m_retiredPages.push_back(m_currentPage);
    m_currentPage = nullptr;
    m_currentOffset = 0;

    sm_pageManager[(size_t)m_allocationType].DiscardPages(FenceID, m_retiredPages);
    m_retiredPages.clear();

    sm_pageManager[(size_t)m_allocationType].FreeLargePages(FenceID, m_largePageList);
    m_largePageList.clear();
}

LinearBuffer LinearAllocator::allocateLargePage(size_t SizeInBytes)
{
    LinearAllocationPage* OneOff = sm_pageManager[(size_t)m_allocationType].CreateNewPage(SizeInBytes);
    m_largePageList.push_back(OneOff);

    LinearBuffer ret(*OneOff, 0, SizeInBytes);
    ret.dataPtr = OneOff->m_CpuVirtualAddress;
    ret.gpuAddress = OneOff->m_GpuVirtualAddress;

    return ret;
}

LinearBuffer LinearAllocator::Allocate(size_t SizeInBytes, size_t Alignment)
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
        m_currentPage = sm_pageManager[(size_t)m_allocationType].RequestPage();
        m_currentOffset = 0;
    }

    LinearBuffer ret(*m_currentPage, m_currentOffset, AlignedSize);
    ret.dataPtr = (uint8_t*)m_currentPage->m_CpuVirtualAddress + m_currentOffset;
    ret.gpuAddress = m_currentPage->m_GpuVirtualAddress + m_currentOffset;

    m_currentOffset += AlignedSize;

    return ret;
}