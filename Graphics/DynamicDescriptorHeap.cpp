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
    std::mutex DynamicDescriptorHeap::sm_mutex;
    std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> DynamicDescriptorHeap::sm_descriptorHeapPool[2];
    std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>> DynamicDescriptorHeap::sm_retiredDescriptorHeaps[2];
    std::queue<ID3D12DescriptorHeap*> DynamicDescriptorHeap::sm_availableDescriptorHeaps[2];

    // Request to available Descriptor Heap then retired.
    ID3D12DescriptorHeap* DynamicDescriptorHeap::requestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType)
    {
        std::lock_guard<std::mutex> LockGuard(sm_mutex);

        uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

        // Fully mobilize Descriptor Available Heaps.
		{
			while (!sm_retiredDescriptorHeaps[idx].empty() && device::g_commandQueueManager.IsFenceComplete(sm_retiredDescriptorHeaps[idx].front().first))
			{
				sm_availableDescriptorHeaps[idx].push(sm_retiredDescriptorHeaps[idx].front().second);
				sm_retiredDescriptorHeaps[idx].pop();
			}
		}

        // If There was availabled retiredDescriptor, then return it.
        if (!sm_availableDescriptorHeaps[idx].empty())
        {
            ID3D12DescriptorHeap* HeapPtr = sm_availableDescriptorHeaps[idx].front();
            sm_availableDescriptorHeaps[idx].pop();
            return HeapPtr;
        }
        else
        {
            D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
            HeapDesc.Type = HeapType;
            HeapDesc.NumDescriptors = kNumDescriptorsPerHeap;
            HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            HeapDesc.NodeMask = 1;
            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> HeapPtr;
            ASSERT_HR(device::g_pDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&HeapPtr)));

#if defined(_DEBUG)
            wchar_t DescriptorHeapName[40];
            swprintf(DescriptorHeapName, _countof(DescriptorHeapName), L"DynamicDescriptorHeap(num:1024) %zu", sm_descriptorHeapPool[idx].size());
            HeapPtr->SetName(DescriptorHeapName);
#endif

            sm_descriptorHeapPool[idx].emplace_back(HeapPtr);
            return HeapPtr.Get();
        }
    }

    void DynamicDescriptorHeap::discardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValue, const std::vector<ID3D12DescriptorHeap*>& UsedHeaps)
    {
        uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

        std::lock_guard<std::mutex> LockGuard(sm_mutex);

		for (auto iter = UsedHeaps.begin(); iter != UsedHeaps.end(); ++iter)
		{
			sm_retiredDescriptorHeaps[idx].push(std::make_pair(FenceValue, *iter)); // with record fence value.
		}
    }

    void DynamicDescriptorHeap::retireCurrentHeap()
    {
        // Don't retire unused heaps.
        if (m_currentOffset == 0)
        {
            ASSERT(m_pCurrentHeap == nullptr);
            return;
        }

        ASSERT(m_pCurrentHeap != nullptr);
        m_pRetiredHeaps.push_back(m_pCurrentHeap);
        m_pCurrentHeap = nullptr;
        m_currentOffset = 0;
    }

    void DynamicDescriptorHeap::retireUsedHeaps(uint64_t fenceValue)
    {
        discardDescriptorHeaps(m_descriptorType, fenceValue, m_pRetiredHeaps);
        m_pRetiredHeaps.clear();
    }

    void DynamicDescriptorHeap::CleanupUsedHeaps(uint64_t fenceValue)
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
            ASSERT(m_currentOffset == 0);
            m_pCurrentHeap = requestDescriptorHeap(m_descriptorType);
            m_firstDescriptor = DescriptorHandle
            (
                m_pCurrentHeap->GetCPUDescriptorHandleForHeapStart(),
                m_pCurrentHeap->GetGPUDescriptorHandleForHeapStart()
            );
        }

        return m_pCurrentHeap;
    }

    //                                                                    16fed   cba9   8765   4321  
    //                                                         StaleParams ¡Û¡Û¡Û¡Û | ¡Û¡Û¡Û¡Û | ¡Û¡Û¡Û¡Û | ¡Û¡Û¡Û¡Û
    // m_rootDescriptorTable[kMaxNumDescriptorTables] DescriptorTableCache ¡Û¡Û¡Û¡Û | ¡Û¡Û¡Û¡Û | ¡Û¡Û¡Û¡Û | ¡Û¡Û¡Û¡Û
    uint32_t DynamicDescriptorHeap::DescriptorHandleCache::ComputeStagedSize()
    {
        // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
        uint32_t NeededSpace = 0;
        uint32_t RootIndex;
        uint32_t StaleParams = m_staleRootParamsBitMap;

        // If hit BitScan, record to RootIndex
        while (_BitScanForward((unsigned long*)&RootIndex, StaleParams))
        {
            StaleParams ^= (1 << RootIndex);

            uint32_t MaxSetHandle;
            ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, m_rootDescriptorTable[RootIndex].assignedHandlesBitMap),
                "Root entry marked as stale but has no stale descriptors");

            NeededSpace += MaxSetHandle + 1;
        }
        return NeededSpace;
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindStaleTables
    (
        D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t DescriptorSize,
        DescriptorHandle DestHandleStart, ID3D12GraphicsCommandList* CmdList,
        void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)
    )
    {
        uint32_t StaleParamCount = 0;
        uint32_t tableSize[DescriptorHandleCache::kMaxNumDescriptorTables];
        uint32_t RootIndices[DescriptorHandleCache::kMaxNumDescriptorTables];
        uint32_t NeededSpace = 0;
        uint32_t RootIndex;

        // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
        uint32_t StaleParams = m_staleRootParamsBitMap;
        while (_BitScanForward((unsigned long*)&RootIndex, StaleParams))
        {
            RootIndices[StaleParamCount] = RootIndex;
            StaleParams ^= (1 << RootIndex);

            uint32_t MaxSetHandle;
            ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, m_rootDescriptorTable[RootIndex].assignedHandlesBitMap),
                "Root entry marked as stale but has no stale descriptors");

            NeededSpace += MaxSetHandle + 1;
            tableSize[StaleParamCount] = MaxSetHandle + 1;

            ++StaleParamCount;
        }

        ASSERT(StaleParamCount <= DescriptorHandleCache::kMaxNumDescriptorTables,
            "We're only equipped to handle so many descriptor tables");

        m_staleRootParamsBitMap = 0;

        static const uint32_t kMaxDescriptorsPerCopy = 16;
        UINT NumDestDescriptorRanges = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[kMaxDescriptorsPerCopy];
        UINT pDestDescriptorRangeSizes[kMaxDescriptorsPerCopy];

        UINT NumSrcDescriptorRanges = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangeStarts[kMaxDescriptorsPerCopy];
        UINT pSrcDescriptorRangeSizes[kMaxDescriptorsPerCopy];

        for (uint32_t i = 0; i < StaleParamCount; ++i)
        {
            RootIndex = RootIndices[i];

            (CmdList->*SetFunc)(RootIndex, DestHandleStart.GetGpuHandle());

            DescriptorTableCache& RootDescTable = m_rootDescriptorTable[RootIndex];

            D3D12_CPU_DESCRIPTOR_HANDLE* SrcHandles = RootDescTable.pTableStart;
            uint64_t SetHandles = (uint64_t)RootDescTable.assignedHandlesBitMap;
            D3D12_CPU_DESCRIPTOR_HANDLE CurDest = DestHandleStart.GetCpuHandle();
            DestHandleStart += tableSize[i] * DescriptorSize;

            unsigned long SkipCount;
            while (_BitScanForward64(&SkipCount, SetHandles))
            {
                // Skip over unset descriptor handles
                SetHandles >>= SkipCount;
                SrcHandles += SkipCount;
                CurDest.ptr += SkipCount * DescriptorSize;

                unsigned long DescriptorCount;
                _BitScanForward64(&DescriptorCount, ~SetHandles);
                SetHandles >>= DescriptorCount;

                // If we run out of temp room, copy what we've got so far
                if (kMaxDescriptorsPerCopy < NumSrcDescriptorRanges + DescriptorCount)
                {
                    device::g_pDevice->CopyDescriptors
                    (
                        NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
                        NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
                        Type
                    );

                    NumSrcDescriptorRanges = 0;
                    NumDestDescriptorRanges = 0;
                }

                // Setup destination range
                pDestDescriptorRangeStarts[NumDestDescriptorRanges] = CurDest;
                pDestDescriptorRangeSizes[NumDestDescriptorRanges] = DescriptorCount;
                ++NumDestDescriptorRanges;

                // Setup source ranges (one descriptor each because we don't assume they are contiguous)
                for (uint32_t j = 0; j < DescriptorCount; ++j)
                {
                    pSrcDescriptorRangeStarts[NumSrcDescriptorRanges] = SrcHandles[j];
                    pSrcDescriptorRangeSizes[NumSrcDescriptorRanges] = 1;
                    ++NumSrcDescriptorRanges;
                }

                // Move the destination pointer forward by the number of descriptors we will copy
                SrcHandles  += DescriptorCount;
                CurDest.ptr += DescriptorCount * DescriptorSize;
            }
        }

        device::g_pDevice->CopyDescriptors
        (
            NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
            NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
            Type
        );
    }

    void DynamicDescriptorHeap::copyAndBindStagedTables
    (
        DescriptorHandleCache& HandleCache, ID3D12GraphicsCommandList* CmdList,
        void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)
    )
    {
        uint32_t NeededSize = HandleCache.ComputeStagedSize();
        if (!hasSpace(NeededSize))
        {
            retireCurrentHeap();
            unbindAllValid();
            NeededSize = HandleCache.ComputeStagedSize();
        }

        // This can trigger the creation of a new heap
        m_owningContext.SetDescriptorHeap(m_descriptorType, getHeapPointer());
        HandleCache.CopyAndBindStaleTables(m_descriptorType, m_descriptorSize, allocate(NeededSize), CmdList, SetFunc);
    }

    void DynamicDescriptorHeap::unbindAllValid(void)
    {
        m_graphicsHandleCache.UnbindAllValid();
        m_computeHandleCache.UnbindAllValid();
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    {
        if (!hasSpace(1))
        {
            retireCurrentHeap();
            unbindAllValid();
        }

        m_owningContext.SetDescriptorHeap(m_descriptorType, getHeapPointer());

        DescriptorHandle DestHandle = m_firstDescriptor + m_currentOffset * m_descriptorSize;
        m_currentOffset += 1;

        device::g_pDevice->CopyDescriptorsSimple(1, DestHandle.GetCpuHandle(), Handle, m_descriptorType);

        return DestHandle.GetGpuHandle();
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::UnbindAllValid()
    {
        m_staleRootParamsBitMap = 0;

        unsigned long TableParams = m_rootDescriptorTablesBitMap;
        unsigned long RootIndex;

        while (_BitScanForward(&RootIndex, TableParams))
        {
            TableParams ^= (1 << RootIndex);
			if (m_rootDescriptorTable[RootIndex].assignedHandlesBitMap != 0)
			{
				m_staleRootParamsBitMap |= (1 << RootIndex);
			}
        }
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::StageDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
    {
        // RootIndex = 0x04, UINT offset = 0x00, UINT NumHandles = 0x06, const D3D12_CPU_DESCRIPTOR[] = SSAO, Shadow SRVs
        ASSERT(((1 << RootIndex) & m_rootDescriptorTablesBitMap) != 0, "Root parameter is not a CBV_SRV_UAV descriptor table");
        ASSERT(Offset + NumHandles <= m_rootDescriptorTable[RootIndex].tableSize);

        DescriptorTableCache& TableCache = m_rootDescriptorTable[RootIndex];

        D3D12_CPU_DESCRIPTOR_HANDLE* CopyDest = TableCache.pTableStart + Offset;

		for (UINT i = 0; i < NumHandles; ++i)
		{
			CopyDest[i] = Handles[i];
		}

        TableCache.assignedHandlesBitMap |= ((1 << NumHandles) - 1) << Offset;
        m_staleRootParamsBitMap          |= (1 << RootIndex);
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE Type, const RootSignature& RootSig)
    {
        ASSERT(RootSig.m_numRootParameters <= 16, "Maybe we need to support something greater");

        m_staleRootParamsBitMap = 0;
		m_rootDescriptorTablesBitMap =
		(
			Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
			RootSig.m_staticSamplerTableBitMap : RootSig.m_descriptorTableBitMap
		);

        UINT CurrentOffset = 0;

        unsigned long TableBitMap = m_rootDescriptorTablesBitMap;
        unsigned long RootIndex;

        while (_BitScanForward(&RootIndex, TableBitMap))
        {
            TableBitMap ^= (1 << RootIndex);

            UINT tableSize = RootSig.m_descriptorTableSize[RootIndex];
            ASSERT(tableSize > 0);

            DescriptorTableCache& RootDescriptorTable = m_rootDescriptorTable[RootIndex];
            RootDescriptorTable.assignedHandlesBitMap = 0;
            RootDescriptorTable.pTableStart           = m_handleCache + CurrentOffset;
            RootDescriptorTable.tableSize             = tableSize;

            CurrentOffset += tableSize;
        }

        m_maxCachedDescriptors = CurrentOffset;

        ASSERT(m_maxCachedDescriptors <= kMaxNumDescriptors, "Exceeded user-supplied maximum cache size");
    }
}