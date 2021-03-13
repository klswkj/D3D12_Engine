#include "stdafx.h"
#include "Device.h"
#include "CommandQueueManager.h"
#include "LinearAllocator.h"

// Container

// static class LinearAllocationPageManager sm_pageManager ( GPU-Writable, CPU-Writable )
// ¦§ std::vector<std::unique_ptr<LinearAllocationPage>> m_pLinearAllocationPagePool;
// ¦§ std::queue<LinearAllocationPage*> m_availablePages;
// ¦§ std::queue<std::pair<uint64_t, LinearAllocationPage*>> m_retiredPages;
// ¦¦ std::queue<std::pair<uint64_t, LinearAllocationPage*>> m_deferredDeletionQueue;
//                                       ¦¢
//                                       ¦¢
//               GPU-Writable                                                   CPU-Writable
//  class LinearAllocator                                         class LinearAllocator
//  ¦§ std::vector<LinearAllocationPage*> m_retiredPages;          ¦§ std::vector<LinearAllocationPage*> m_retiredPages;
//  ¦¦ std::vector<LinearAllocationPage*> m_largePageList;         ¦¦ std::vector<LinearAllocationPage*> m_largePageList;

static size_t LinearAllocateCount = 0ul;

#pragma region LINEAR_ALLOCATION_PAGE_MANAGER

LinearAllocatorType LinearAllocator::LinearAllocationPageManager::sm_autoType = LinearAllocatorType::GPU_WRITEABLE;

LinearAllocator::LinearAllocationPageManager LinearAllocator::sm_pageManager[2];

LinearAllocator::LinearAllocationPageManager::LinearAllocationPageManager()
    :
    m_CS({})
{
    m_allocationType = sm_autoType;
    sm_autoType = (LinearAllocatorType)((uint32_t)sm_autoType + 1);
    ASSERT(sm_autoType <= LinearAllocatorType::COUNT);

    InitializeCriticalSection(&m_CS);
}

LinearAllocator::LinearAllocationPageManager::~LinearAllocationPageManager()
{
    DeleteCriticalSection(&m_CS);
}

LinearAllocationPage* LinearAllocator::LinearAllocationPageManager::RequestPage()
{
    LinearAllocationPage* PagePtr = nullptr;

    EnterCriticalSection(&m_CS);

    while (!m_retiredPages.empty() && device::g_commandQueueManager.IsFenceComplete(m_retiredPages.front().first))
    {
        m_availablePages.push(m_retiredPages.front().second);
        m_retiredPages.pop();
    }

    if (!m_availablePages.empty())
    {
        PagePtr = m_availablePages.front();
        m_availablePages.pop();
    }
    else
    {
        PagePtr = CreateNewPage(0);
        m_pLinearAllocationPagePool.emplace_back(PagePtr);
    }

    LeaveCriticalSection(&m_CS);
    return PagePtr;
}

void LinearAllocator::LinearAllocationPageManager::DiscardPages(uint64_t FenceValue, const std::vector<LinearAllocationPage*>& UsedPages)
{
    EnterCriticalSection(&m_CS);

    for (auto iter = UsedPages.begin(); iter != UsedPages.end(); ++iter)
    {
        m_retiredPages.push({ FenceValue, *iter });
    }

    LeaveCriticalSection(&m_CS);
}

// -> CommandContext::Finish(bool WaitForCompletion)
// -> LinearAllocator::CleanupUsedPages(uint64_t FenceID)
// -> LinearAllocator::LinearAllocationPageManager::FreeLargePages(uint64_t FenceValue, const std::vector<LinearAllocationPage*>& LargePages)
void LinearAllocator::LinearAllocationPageManager::FreeLargePages(uint64_t FenceValue, const std::vector<LinearAllocationPage*>& LargePages)
{
    EnterCriticalSection(&m_CS);

    for (auto pLinearAllocationPage = LargePages.begin(); pLinearAllocationPage != LargePages.end(); ++pLinearAllocationPage)
    {
        (*pLinearAllocationPage)->SafeUnmap();
        m_deferredDeletionQueue.push({ FenceValue, *pLinearAllocationPage });
    }

    while (!m_deferredDeletionQueue.empty() && device::g_commandQueueManager.IsFenceComplete(m_deferredDeletionQueue.front().first))
    {
        delete m_deferredDeletionQueue.front().second;
        m_deferredDeletionQueue.pop();
    }

    LeaveCriticalSection(&m_CS);
}

LinearAllocationPage* LinearAllocator::LinearAllocationPageManager::CreateNewPage(size_t PageSize /* = 0*/)
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
        device::g_pDevice->CreateCommittedResource
        (
            &HeapProps, D3D12_HEAP_FLAG_NONE,
            &ResourceDesc, DefaultUsage, nullptr, IID_PPV_ARGS(&pBuffer)
        )
    );

#if defined(_DEBUG)
    wchar_t ResourceName[32];
    swprintf(ResourceName, _countof(ResourceName), L"LinearAllocateResource %zu", ++LinearAllocateCount);
    pBuffer->SetName(ResourceName);
#endif

    return new LinearAllocationPage(pBuffer, DefaultUsage);
}

#pragma endregion LINEAR_ALLOCATION_PAGE_MANAGER

#pragma region LINEAR_ALLOCATOR

void LinearAllocator::CleanupUsedPages(uint64_t FenceID)
{
    if (m_currentPage == nullptr)
    {
        return;
    }

    m_retiredPages.push_back(m_currentPage);
    m_currentPage   = nullptr;
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
    ret.pData = OneOff->m_CpuVirtualAddress;
    ret.GPUAddress = OneOff->m_GpuVirtualAddress;

    return ret;
}

LinearBuffer LinearAllocator::Allocate(size_t SizeInBytes, size_t Alignment /*= 256ul*/)
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
    ret.pData = (uint8_t*)m_currentPage->m_CpuVirtualAddress + m_currentOffset;
    ret.GPUAddress = m_currentPage->m_GpuVirtualAddress + m_currentOffset;

    m_currentOffset += AlignedSize;

    return ret;
}

#pragma endregion LINEAR_ALLOCATOR