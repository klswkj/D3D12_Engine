#include "stdafx.h"
#include "Device.h"
#include "RootSignature.h"
#include "DynamicDescriptorHeap.h"
#include "DescriptorHandle.h"
#include "CommandQueueManager.h"
#include "CommandContext.h"

// namespace-device
// ID3D12Device* g_pDevice;
// CommandQueueManager g_commandQueueManager;

namespace custom
{
    // std::mutex DynamicDescriptorHeap::sm_mutex;
    custom::RAII_CS                                           DynamicDescriptorHeap::sm_CS;

    std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> DynamicDescriptorHeap::sm_descriptorHeapPool      [2];
    std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>>    DynamicDescriptorHeap::sm_retiredDescriptorHeaps  [2];
    std::queue<ID3D12DescriptorHeap*>                         DynamicDescriptorHeap::sm_availableDescriptorHeaps[2];

    // Request to available Descriptor Heap then retired.
    STATIC ID3D12DescriptorHeap* DynamicDescriptorHeap::requestDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE heapType)
    {
        uint32_t HeapTypeIndex = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

        Scoped_CS ScopedCS(sm_CS);

        // Fully mobilize Descriptor Available Heaps.
		{
			while (!sm_retiredDescriptorHeaps[HeapTypeIndex].empty() && device::g_commandQueueManager.IsFenceComplete(sm_retiredDescriptorHeaps[HeapTypeIndex].front().first))
			{
				sm_availableDescriptorHeaps[HeapTypeIndex].push(sm_retiredDescriptorHeaps[HeapTypeIndex].front().second);
				sm_retiredDescriptorHeaps[HeapTypeIndex].pop();
			}
		}

        // If There was availabled retiredDescriptor, then return it.
        if (!sm_availableDescriptorHeaps[HeapTypeIndex].empty())
        {
            ID3D12DescriptorHeap* HeapPtr = sm_availableDescriptorHeaps[HeapTypeIndex].front();
            sm_availableDescriptorHeaps[HeapTypeIndex].pop();
            return HeapPtr;
        }
        else
        {
            D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
            HeapDesc.Type           = heapType;
            HeapDesc.NumDescriptors = kNumDescriptorsPerHeap;
            HeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            HeapDesc.NodeMask       = 1;

            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> HeapPtr;
            ASSERT_HR(device::g_pDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&HeapPtr)));

#if defined(_DEBUG)
            wchar_t DescriptorHeapName[40];
            swprintf(DescriptorHeapName, _countof(DescriptorHeapName), L"DynamicDescriptorHeap(num:1024) %zu", sm_descriptorHeapPool[HeapTypeIndex].size());
            HeapPtr->SetName(DescriptorHeapName);
#endif
            sm_descriptorHeapPool[HeapTypeIndex].emplace_back(HeapPtr);
            return HeapPtr.Get();
        }
    }

    STATIC void DynamicDescriptorHeap::discardDescriptorHeaps(const D3D12_DESCRIPTOR_HEAP_TYPE heapType, const uint64_t fenceValue, const std::vector<ID3D12DescriptorHeap*>& usedHeaps)
    {
        uint32_t HeapTypeIndex = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

        Scoped_CS ScopedCS(sm_CS);

		for (auto iter = usedHeaps.begin(); iter != usedHeaps.end(); ++iter)
		{
			sm_retiredDescriptorHeaps[HeapTypeIndex].push(std::make_pair(fenceValue, *iter)); // with record fence value.
		}
    }

    void DynamicDescriptorHeap::retireCurrentHeap()
    {
        // Don't retire unused heaps.
        if (m_currentOffset == 0u)
        {
            ASSERT(m_pCurrentHeap == nullptr);
            return;
        }

        ASSERT(m_pCurrentHeap != nullptr);
        m_pRetiredHeaps.push_back(m_pCurrentHeap);
        m_pCurrentHeap  = nullptr;
        m_currentOffset = 0u;
    }

    void DynamicDescriptorHeap::retireUsedHeaps(const uint64_t fenceValue)
    {
        discardDescriptorHeaps(m_descriptorType, fenceValue, m_pRetiredHeaps);
        m_pRetiredHeaps.clear();
    }

    // CommandContext.m_GPUTaskFiberContexts[i].DynamicViewDescriptorHeaps.CleanupUsedHeaps(m_LastExecuteFenceValue);
    // CommandContext.m_GPUTaskFiberContexts[i].DynamicSamplerDescriptorHeaps.CleanupUsedHeaps(m_LastExecuteFenceValue);
    void DynamicDescriptorHeap::CleanupUsedHeaps(const uint64_t fenceValue)
    {
        retireCurrentHeap();
        retireUsedHeaps(fenceValue);
        m_graphicsHandleCache.ClearCache();
        m_computeHandleCache.ClearCache();
    }

    inline ID3D12DescriptorHeap* DynamicDescriptorHeap::getHeapPointer()
    {
        if (m_pCurrentHeap == nullptr)
        {
            ASSERT(m_currentOffset == 0u);

            m_pCurrentHeap    = requestDescriptorHeap(m_descriptorType);
            m_firstDescriptor = DescriptorHandle
            (
                m_pCurrentHeap->GetCPUDescriptorHandleForHeapStart(),
                m_pCurrentHeap->GetGPUDescriptorHandleForHeapStart()
            );
        }

        return m_pCurrentHeap;
    }

    //                                                                     fedc   ba98   7654   3210  
    //                                                         StaleParams ¡Û¡Û¡Û¡Û | ¡Û¡Û¡Û¡Û | ¡Û¡Û¡Û¡Û | ¡Û¡Û¡Û¡Û
    // m_rootDescriptorTable[kMaxNumDescriptorTables] DescriptorTableCache ¡Û¡Û¡Û¡Û | ¡Û¡Û¡Û¡Û | ¡Û¡Û¡Û¡Û | ¡Û¡Û¡Û¡Û
    uint32_t DynamicDescriptorHeap::DescriptorHandleCache::ComputeStagedSize() const
    {
        // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
        uint32_t NeededSpace     = 0u;
        uint32_t RootIndex       = 0u;
        uint32_t TempStaleParams = m_staleRootParamsBitMap;

        // If hit BitScan, record to rootIndex
        while (::_BitScanForward((unsigned long*)&RootIndex, TempStaleParams))
        {
            TempStaleParams ^= (1 << RootIndex);

            uint32_t MaxSetHandle = 0u;
            ASSERT(TRUE == ::_BitScanReverse((unsigned long*)&MaxSetHandle, m_rootDescriptorTableCache[RootIndex].assignedHandlesBitMap),
                "Root entry marked as stale but has no stale descriptors");

            NeededSpace += MaxSetHandle + 1;
        }
        return NeededSpace;
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::SetOwnedRootDescriptorTables
    (
        const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t descriptorSize,
        DescriptorHandle destHandleStart, ID3D12GraphicsCommandList* const pCmdList,
        void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* pSetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)
    )
    {
        uint32_t TableSize[DescriptorHandleCache::kMaxNumDescriptorTables]   = {};
        uint32_t RootIndices[DescriptorHandleCache::kMaxNumDescriptorTables] = {};

        // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
        uint32_t TempStaleParams = m_staleRootParamsBitMap;
        uint32_t StaleParamCount = 0u;

        uint32_t NeededSpace     = 0u;
        uint32_t RootIndex       = 0u;

        while (::_BitScanForward((unsigned long*)&RootIndex, TempStaleParams))
        {
            RootIndices[StaleParamCount] = RootIndex;
            TempStaleParams ^= (1 << RootIndex);

            uint32_t MaxSetHandle = 0u;

            ASSERT(TRUE == ::_BitScanReverse((unsigned long*)&MaxSetHandle, m_rootDescriptorTableCache[RootIndex].assignedHandlesBitMap),
                "Root entry marked as stale but has no stale descriptors");

            NeededSpace               += MaxSetHandle + 1;
            TableSize[StaleParamCount] = MaxSetHandle + 1;

            ++StaleParamCount;
        }

        ASSERT(StaleParamCount <= DescriptorHandleCache::kMaxNumDescriptorTables,
            "We're only equipped to handle so many descriptor tables");

        m_staleRootParamsBitMap = 0;

        static const uint32_t kMaxDescriptorsPerCopy = 16u;

        UINT NumDestDescriptorRanges = 0u;
        D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[kMaxDescriptorsPerCopy] = {};
        UINT                        pDestDescriptorRangeSizes[kMaxDescriptorsPerCopy]  = {};

        UINT NumSrcDescriptorRanges = 0u;
        D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangeStarts[kMaxDescriptorsPerCopy] = {};
        UINT                        pSrcDescriptorRangeSizes[kMaxDescriptorsPerCopy]  = {};

        for (uint32_t i = 0u; i < StaleParamCount; ++i)
        {
            RootIndex = RootIndices[i];

            (pCmdList->*pSetFunc)(RootIndex, destHandleStart.GetGpuHandle());

            D3D12_CPU_DESCRIPTOR_HANDLE* SrcHandles = m_rootDescriptorTableCache[RootIndex].pTableStart;
            uint64_t SetHandles           = (uint64_t)m_rootDescriptorTableCache[RootIndex].assignedHandlesBitMap;

            D3D12_CPU_DESCRIPTOR_HANDLE CurDest = destHandleStart.GetCpuHandle();
            destHandleStart += TableSize[i] * descriptorSize;

            unsigned long SkipCount;

            while (::_BitScanForward64(&SkipCount, SetHandles))
            {
                // Skip over unset descriptor handles
                SetHandles >>= SkipCount;
                SrcHandles += SkipCount;
                CurDest.ptr += SkipCount * (size_t)descriptorSize;

                unsigned long DescriptorCount;
                ::_BitScanForward64(&DescriptorCount, ~SetHandles);
                SetHandles >>= DescriptorCount;

                // If we run out of temp room, copy what we've got so far
                if (kMaxDescriptorsPerCopy < NumSrcDescriptorRanges + DescriptorCount)
                {
                    device::g_pDevice->CopyDescriptors
                    (
                        NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
                        NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
                        type
                    );

                    NumSrcDescriptorRanges  = 0U;
                    NumDestDescriptorRanges = 0U;
                }

                // Setup destination range
                pDestDescriptorRangeStarts[NumDestDescriptorRanges] = CurDest;
                pDestDescriptorRangeSizes[NumDestDescriptorRanges]  = DescriptorCount;
                ++NumDestDescriptorRanges;

                // Setup source ranges (one descriptor each because we don't assume they are contiguous)
                for (uint32_t j = 0u; j < DescriptorCount; ++j)
                {
                    pSrcDescriptorRangeStarts[NumSrcDescriptorRanges] = SrcHandles[j];
                    pSrcDescriptorRangeSizes[NumSrcDescriptorRanges]  = 1U;
                    ++NumSrcDescriptorRanges;
                }

                // Move the destination pointer forward by the number of descriptors we will copy
                SrcHandles  += DescriptorCount;
                CurDest.ptr += DescriptorCount * (size_t)descriptorSize;
            }
        }

        device::g_pDevice->CopyDescriptors
        (
            NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
            NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
            type
        );
    }

    void DynamicDescriptorHeap::SetCachedRootDescriptorTables
    (
        DescriptorHandleCache& handleCache, ID3D12GraphicsCommandList* const pCmdList, const uint8_t commandIndex,
        void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* pSetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)
    )
    {
        uint32_t NeededSize = handleCache.ComputeStagedSize();
        if (!hasSpace(NeededSize))
        {
            retireCurrentHeap();
            unbindAllValid();
            NeededSize = handleCache.ComputeStagedSize();
        }

        // This can trigger the creation of a new heap
        m_owningContext.setDescriptorHeap(m_descriptorType, getHeapPointer(), commandIndex);
        handleCache.SetOwnedRootDescriptorTables(m_descriptorType, m_descriptorSize, allocateDescriptorHandle(NeededSize), pCmdList, pSetFunc);
    }

    void DynamicDescriptorHeap::unbindAllValid()
    {
        m_graphicsHandleCache.UnbindAllValid();
        m_computeHandleCache.UnbindAllValid();
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::UploadDirect(const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, const uint8_t commandIndex)
    {
        if (!hasSpace(1))
        {
            retireCurrentHeap();
            unbindAllValid();
        }

        m_owningContext.setDescriptorHeap(m_descriptorType, getHeapPointer(), commandIndex);

        DescriptorHandle DestHandle = m_firstDescriptor + m_currentOffset * m_descriptorSize;
        m_currentOffset += 1;

        device::g_pDevice->CopyDescriptorsSimple(1, DestHandle.GetCpuHandle(), CPUHandle, m_descriptorType);

        return DestHandle.GetGpuHandle();
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::UnbindAllValid()
    {
        m_staleRootParamsBitMap = 0;

        unsigned long TableParams = m_rootDescriptorTablesBitMap;
        unsigned long RootIndex;

        while (::_BitScanForward(&RootIndex, TableParams))
        {
            TableParams ^= (1ul << RootIndex);
			if (m_rootDescriptorTableCache[RootIndex].assignedHandlesBitMap != 0u)
			{
				m_staleRootParamsBitMap |= (1ul << RootIndex);
			}
        }
    }
    // Pass::-> CommandContext::SetDynmaicDescriptors(GPUResource.UAV()'s Array);
    // CommandContext::SetDynamicDescriptors (DynamicViewDescriptorHeaps, DynamicSamplerDescriptorHeaps)
    // ->
    // DynamicDescriptorHeap::SetGraphicsDescriptorHandles,
    // DynamicDescriptorHeap::SetComputeDescriptorHandles
    void DynamicDescriptorHeap::DescriptorHandleCache::StageDescriptorHandles(const UINT rootIndex, const UINT Offset, const UINT numCPUHandles, const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandles[])
    {
        // rootIndex = 0x04, UINT offset = 0x00, UINT numCPUHandles = 0x06, const D3D12_CPU_DESCRIPTOR[] = SSAO, Shadow SRVs
        ASSERT(((1 << rootIndex) & m_rootDescriptorTablesBitMap) != 0u, "Root parameter is not a CBV_SRV_UAV descriptor table");
        ASSERT(Offset + numCPUHandles <= m_rootDescriptorTableCache[rootIndex].tableSize);

        D3D12_CPU_DESCRIPTOR_HANDLE* CopyDest = m_rootDescriptorTableCache[rootIndex].pTableStart + Offset;

		for (UINT i = 0U; i < numCPUHandles; ++i)
		{
			CopyDest[i] = CPUHandles[i];
		}

        m_rootDescriptorTableCache[rootIndex].assignedHandlesBitMap |= ((1U << numCPUHandles) - 1U) << Offset;
        m_staleRootParamsBitMap                                     |= (1U << rootIndex);
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(const D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& customRS)
    {
        ASSERT(customRS.m_numRootParameters <= 16, "Maybe we need to support something greater");

        m_staleRootParamsBitMap = 0;
		m_rootDescriptorTablesBitMap =
		(
			type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
			customRS.m_staticSamplerTableBitMap : customRS.m_descriptorTableBitMap
		);

        UINT CurrentOffset = 0;

        unsigned long TempTableBitMap = m_rootDescriptorTablesBitMap;
        unsigned long RootIndex;

        while (::_BitScanForward(&RootIndex, TempTableBitMap))
        {
            TempTableBitMap ^= (1 << RootIndex);

            UINT tableSize = customRS.m_descriptorTableSize[RootIndex];
            ASSERT(tableSize);

            DescriptorTableCache& RootDescriptorTable = m_rootDescriptorTableCache[RootIndex];
            RootDescriptorTable.assignedHandlesBitMap = 0;
            RootDescriptorTable.pTableStart           = m_handleCache + CurrentOffset;
            RootDescriptorTable.tableSize             = tableSize;

            CurrentOffset += tableSize;
        }

        m_maxCachedDescriptors = CurrentOffset;

        ASSERT(m_maxCachedDescriptors <= kMaxNumDescriptors, "Exceeded user-supplied maximum cache size");
    }
}