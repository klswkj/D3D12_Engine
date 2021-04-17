//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once
namespace D3DX12Residency
{
// Note: This library automatically runs in a single-threaded mode if ID3D12Device3 is supported.
#define RESIDENCY_SINGLE_THREADED 0

    // This size can be tuned to your app in order to save space
#define MAX_NUM_CONCURRENT_CMD_LISTS 32

    constexpr float s_MaxResidencySetSize_GrowthAddRatio_RCP = 2.0f;

    namespace Internal
    {
        class CriticalSection
        {
            friend class ScopedLock;
        public:
            CriticalSection()
            {
                assert(::InitializeCriticalSectionAndSpinCount(&CS, 8));
            }

            ~CriticalSection()
            {
                ::DeleteCriticalSection(&CS);
            }

        private:
            CRITICAL_SECTION CS;
        };

        class ScopedLock
        {
        public:
            explicit ScopedLock() 
                : 
                pCS(nullptr) 
            {
            };
            explicit ScopedLock(CriticalSection* pCSIn) : pCS(pCSIn)
            {
                if (pCS)
                {
                    ::EnterCriticalSection(&pCS->CS);
                }
            };

            ~ScopedLock()
            {
                if (pCS)
                {
                    LeaveCriticalSection(&pCS->CS);
                }
            }

        private:
            CriticalSection* pCS;
        };

        // One per Residency Manager
        class SyncManager
        {
        public:
            SyncManager()
            {
                for (uint32_t i = 0; i < ARRAYSIZE(AvailableCommandLists); i++)
                {
                    AvailableCommandLists[i] = false;
                }
            }

            Internal::CriticalSection MaskCriticalSection;

            static const uint32_t sm_UnsetValue = uint32_t(-1);
            // Represents which command lists are currently open for recording
            bool AvailableCommandLists[MAX_NUM_CONCURRENT_CMD_LISTS];
        };

        //Forward Declaration
        class ResidencyManagerInternal;
    }

    // Used to track meta data for each object the app potentially wants
    // to make resident or evict.
    class ManagedObject
    {
    public:
        enum class RESIDENCY_STATUS
        {
            RESIDENT,
            EVICTED
        };

        ManagedObject() 
            :
            m_pUnderlyingD3D12Pageable(nullptr),
            m_PageableBytesSize(0),
            m_eResidencyStatus(RESIDENCY_STATUS::RESIDENT),
            m_LastGPUSyncPoint(0),
            m_LastUsedTimestamp(0)
        {
            memset(m_bCommandListsUsedOn, 0, sizeof(m_bCommandListsUsedOn));
        }

        void Initialize(ID3D12Pageable* pUnderlyingIn, uint64_t ObjectSize, uint64_t InitialGPUSyncPoint = 0)
        {
            ASSERT(m_pUnderlyingD3D12Pageable == nullptr);

            m_pUnderlyingD3D12Pageable = pUnderlyingIn;
            m_PageableBytesSize        = ObjectSize;
            m_LastGPUSyncPoint         = InitialGPUSyncPoint;
        }

        inline bool IsInitialized() { return m_pUnderlyingD3D12Pageable != nullptr; }

        RESIDENCY_STATUS m_eResidencyStatus; // Wether the object is resident or not
        ID3D12Pageable*  m_pUnderlyingD3D12Pageable; // The underlying D3D Object being tracked

        uint64_t m_PageableBytesSize; // The size of the D3D Object in bytes // IMPORTANT!!!!!!
        uint64_t m_LastGPUSyncPoint;
        uint64_t m_LastUsedTimestamp;

        // This is used to track which open command lists this resource is currently used on.
        bool m_bCommandListsUsedOn[MAX_NUM_CONCURRENT_CMD_LISTS];

        // Linked list entry
        LIST_ENTRY m_ListEntry;
    };

    // This represents a set of objects which are referenced by a command list 
    // i.e. every time a resource is bound for rendering, clearing, copy etc.
    // the set must be updated to ensure the it is resident for execution.
    class ResidencySet
    {
        friend class ResidencyManager;
        friend class Internal::ResidencyManagerInternal;
    public:

        static const uint32_t sm_InvalidIndex = (uint32_t)-1;

        ResidencySet() 
            :
            m_CommandListIndex(sm_InvalidIndex),
            m_MaxResidencySetSize(0),
            m_CurrentSetSize(0),
            m_ppManagedObjectSet(nullptr),
            m_bOpen(false),
            m_bOutOfMemory(false),
            m_pMainSyncManager(nullptr)
        {
        };

        ~ResidencySet()
        {
            delete[](m_ppManagedObjectSet);
        }

        // Returns true if the object was inserted, false otherwise
        inline bool Insert(ManagedObject* pManagedObject)
        {
            ASSERT(m_bOpen);
            ASSERT(m_CommandListIndex != sm_InvalidIndex);

            // If we haven't seen this object on this command list mark it
            if (pManagedObject->m_bCommandListsUsedOn[m_CommandListIndex] == false)
            {
                pManagedObject->m_bCommandListsUsedOn[m_CommandListIndex] = true;

                if (m_ppManagedObjectSet == nullptr || 
                   m_MaxResidencySetSize <= m_CurrentSetSize)
                {
                    Realloc();
                }
                if (m_ppManagedObjectSet == nullptr)
                {
                    m_bOutOfMemory = true;
                    return false;
                }

                m_ppManagedObjectSet[m_CurrentSetSize++] = pManagedObject;

                return true;
            }
            else
            {
                return false;
            }
        }

        HRESULT Open()
        {
            Internal::ScopedLock Lock(&m_pMainSyncManager->MaskCriticalSection);

            // It's invalid to open a set that is already open
            if (m_bOpen)
            {
                ASSERT(false); // debug
                return E_INVALIDARG;
            }

            ASSERT(m_CommandListIndex == sm_InvalidIndex);

            bool CommandlistAvailable = false;
            // Find the first available command list by bitscanning
            for (uint32_t i = 0; i < ARRAYSIZE(m_pMainSyncManager->AvailableCommandLists); ++i)
            {
                if (m_pMainSyncManager->AvailableCommandLists[i] == false)
                {
                    m_CommandListIndex = i;
                    m_pMainSyncManager->AvailableCommandLists[i] = true;
                    CommandlistAvailable = true;
                    break;
                }
            }

            if (CommandlistAvailable == false)
            {
                // There are too many open residency sets, consider using less or increasing the value of MAX_NUM_CONCURRENT_CMD_LISTS
                // ASSERT(false);
                return E_OUTOFMEMORY;
            }

            m_CurrentSetSize = 0;

            m_bOpen = true;
            m_bOutOfMemory = false;
            return S_OK;
        }

        HRESULT Close()
        {
            // ASSERT(m_bOpen && !m_bOutOfMemory);

            if (m_bOpen == false)
            {
                ASSERT(false); // Debug
                return E_INVALIDARG;
            }

            if (m_bOutOfMemory == true)
            {
                return E_OUTOFMEMORY;
            }

            for (int i = 0; i < m_CurrentSetSize; i++)
            {
                Remove(m_ppManagedObjectSet[i]);
            }

            ReturnCommandListReservation();

            m_bOpen = false;

            return S_OK;
        }

    private:

        inline void Remove(ManagedObject* pObject)
        {
            pObject->m_bCommandListsUsedOn[m_CommandListIndex] = false;
        }

        inline void ReturnCommandListReservation()
        {
            Internal::ScopedLock Lock(&m_pMainSyncManager->MaskCriticalSection);

            m_pMainSyncManager->AvailableCommandLists[m_CommandListIndex] = false;
            m_CommandListIndex = ResidencySet::sm_InvalidIndex;
            m_bOpen = false;
        }

        inline void InitializeResidencySet(Internal::SyncManager* pSyncManagerIn)
        {
            ASSERT(m_pMainSyncManager);
            m_pMainSyncManager = pSyncManagerIn;
        }

        inline bool InitializeResidencySet(Internal::SyncManager* pSyncManagerIn, uint32_t maxSize)
        {
            m_pMainSyncManager    = pSyncManagerIn;
            m_MaxResidencySetSize = maxSize;
            m_ppManagedObjectSet  = new ManagedObject*[m_MaxResidencySetSize];

            return m_ppManagedObjectSet != nullptr;
        }

        inline void Realloc()
        {
            m_MaxResidencySetSize = (m_MaxResidencySetSize == 0) ? 4096 : int(m_MaxResidencySetSize + (m_MaxResidencySetSize / s_MaxResidencySetSize_GrowthAddRatio_RCP));
            ManagedObject** ppNewAlloc = new ManagedObject*[m_MaxResidencySetSize];

            if (m_ppManagedObjectSet && ppNewAlloc)
            {
                ::memcpy(ppNewAlloc, m_ppManagedObjectSet, m_CurrentSetSize * sizeof(ManagedObject*));
                delete[](m_ppManagedObjectSet);
            }

            m_ppManagedObjectSet = ppNewAlloc;
        }

        ManagedObject** m_ppManagedObjectSet;

        uint32_t m_CommandListIndex;
        int      m_MaxResidencySetSize;
        int      m_CurrentSetSize; // IMPORTANT!!!!!!
        bool     m_bOpen;
        bool     m_bOutOfMemory;

        Internal::SyncManager* m_pMainSyncManager;
    };

    namespace Internal
    {
        struct FenceStructure
        {
            FenceStructure(uint64_t initFenceValue) 
                : 
                pFence(nullptr), 
                FenceValue(initFenceValue)
            {
                listHelper::InitializeListHead(&ListEntry);
            };

            HRESULT Initialize(ID3D12Device* pDevice)
            {
                HRESULT hr = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));
                ASSERT_HR(hr);

                return hr;
            }

            void Destroy()
            {
                if (pFence)
                {
                    pFence->Release();
                    pFence = nullptr;
                }
            }

            HRESULT GPUWait(ID3D12CommandQueue* pQueue)
            {
                HRESULT hr = pQueue->Wait(pFence, FenceValue);
                ASSERT_HR(hr);
                return hr;
            }

            HRESULT GPUSignal(ID3D12CommandQueue* pQueue)
            {
                HRESULT hr = pQueue->Signal(pFence, FenceValue);
                ASSERT_HR(hr);
                return hr;
            }

            ID3D12Fence*      pFence;
            volatile uint64_t FenceValue;
            LIST_ENTRY        ListEntry;
        };

        // Represents a time on a particular queue that a resource was used
        struct QueueSyncPoint
        {
            QueueSyncPoint() 
                : 
                pFence(nullptr), 
                LastUsedValue(0) 
            {
            };

            inline bool IsCompleted() { return LastUsedValue <= pFence->pFence->GetCompletedValue(); }

            inline void WaitForCompletion(HANDLE Event)
            {
                ASSERT_HR(pFence->pFence->SetEventOnCompletion(LastUsedValue, Event));
                ASSERT_HR(WaitForSingleObject(Event, INFINITE));
            }

            FenceStructure* pFence;
            uint64_t        LastUsedValue;
        };

        struct DeviceWideSyncPoint
        {
            DeviceWideSyncPoint(uint32_t NumQueues, uint64_t Generation) 
                :
                GenerationID(Generation), 
                NumQueueSyncPoints(NumQueues) 
            {
                ListEntry.Flink = ListEntry.Blink = &ListEntry;
            };

            // Create the whole structure in one allocation for locality
            static DeviceWideSyncPoint* CreateSyncPoint(uint32_t NumQueues, uint64_t Generation)
            {
                DeviceWideSyncPoint* pSyncPoint = nullptr;
                const size_t Size = (size_t)(sizeof(DeviceWideSyncPoint) + (sizeof(QueueSyncPoint) * (NumQueues - 1)));

                BYTE* pAlloc = new BYTE[Size];
                if (pAlloc && Size >= sizeof(DeviceWideSyncPoint))
                {
                    pSyncPoint = new (pAlloc) DeviceWideSyncPoint(NumQueues, Generation);
                }

                return pSyncPoint;
            }

            // A device wide fence is completed if all of the queues that were active at that point are completed
            inline bool IsCompleted()
            {
                for (uint32_t i = 0; i < NumQueueSyncPoints; i++)
                {
                    if (pQueueSyncPoints[i].IsCompleted() == false)
                    {
                        return false;
                    }
                }
                return true;
            }

            inline void WaitForCompletion(HANDLE Event)
            {
                for (uint32_t i = 0; i < NumQueueSyncPoints; ++i)
                {
                    if (pQueueSyncPoints[i].IsCompleted() == false)
                    {
                        pQueueSyncPoints[i].WaitForCompletion(Event);
                    }
                }
            }

            const uint64_t GenerationID;
            const uint32_t NumQueueSyncPoints;
            LIST_ENTRY ListEntry;
            // NumQueueSyncPoints QueueSyncPoints will be placed below here
            QueueSyncPoint pQueueSyncPoints[1];
        }; // DeviceWideSyncPoint

        // A Least Recently Used Cache. 
        // Tracks all of the objects requested by the app so that objects
        // that aren't used freqently can get evicted to help the app stay under buget.
        class LRUCache
        {
        public:
            LRUCache() :
                m_NumResidentObjects(0),
                m_NumEvictedObjects(0),
                m_ResidentSize(0)
            {
                listHelper::InitializeListHead(&m_ResidentObjectListHead);
                listHelper::InitializeListHead(&m_EvictedObjectListHead);
            };

            void InsertFromResidentListHead(ManagedObject* pObject)
            {
                if (pObject->m_eResidencyStatus == ManagedObject::RESIDENCY_STATUS::RESIDENT)
                {
                    listHelper::InsertHeadList(&m_ResidentObjectListHead, &pObject->m_ListEntry);
                    m_NumResidentObjects++;
                    m_ResidentSize += pObject->m_PageableBytesSize;
                }
                else
                {
                    listHelper::InsertHeadList(&m_EvictedObjectListHead, &pObject->m_ListEntry);
                    m_NumEvictedObjects++;
                }
            }

            void RemoveFromResidentListHead(ManagedObject* pObject)
            {
                listHelper::RemoveEntryList(&pObject->m_ListEntry);
                if (pObject->m_eResidencyStatus == ManagedObject::RESIDENCY_STATUS::RESIDENT)
                {
                    m_NumResidentObjects--;
                    m_ResidentSize -= pObject->m_PageableBytesSize;
                }
                else
                {
                    m_NumEvictedObjects--;
                }
            }

            // When an object is used by the GPU we move it to the end of the list.
            // This way things closer to the head of the list are the objects which
            // are stale and better candidates for eviction
            void ObjectReferenced(ManagedObject* pObject)
            {
                ASSERT(pObject->m_eResidencyStatus == ManagedObject::RESIDENCY_STATUS::RESIDENT);

                listHelper::RemoveEntryList(&pObject->m_ListEntry);
                listHelper::InsertTailList(&m_ResidentObjectListHead, &pObject->m_ListEntry);
            }

            void MakeStatusResident(ManagedObject* pObject)
            {
                ASSERT(pObject->m_eResidencyStatus == ManagedObject::RESIDENCY_STATUS::EVICTED);

                pObject->m_eResidencyStatus = ManagedObject::RESIDENCY_STATUS::RESIDENT;
                listHelper::RemoveEntryList(&pObject->m_ListEntry);
                listHelper::InsertTailList(&m_ResidentObjectListHead, &pObject->m_ListEntry);

                m_NumEvictedObjects--;
                m_NumResidentObjects++;
                m_ResidentSize += pObject->m_PageableBytesSize;
            }

            void MakeStatusEvict(ManagedObject* pObject)
            {
                ASSERT(pObject->m_eResidencyStatus == ManagedObject::RESIDENCY_STATUS::RESIDENT);

                pObject->m_eResidencyStatus = ManagedObject::RESIDENCY_STATUS::EVICTED;
                listHelper::RemoveEntryList(&pObject->m_ListEntry);
                listHelper::InsertTailList(&m_EvictedObjectListHead, &pObject->m_ListEntry);

                m_NumResidentObjects--;
                m_ResidentSize -= pObject->m_PageableBytesSize;
                m_NumEvictedObjects++;
            }

            // From ResidencyManagerInternal::ProcessPagingWork
            // Evict all of the resident objects used in sync points up to the specficied one (inclusive)
            void TrimToSyncPointInclusive(int64_t CurrentUsage, int64_t CurrentBudget, ID3D12Pageable** EvictionList, uint32_t& NumObjectsToEvict, uint64_t SyncPoint)
            {
                NumObjectsToEvict = 0;

                LIST_ENTRY* pResourceEntry = m_ResidentObjectListHead.Flink;
                while (pResourceEntry != &m_ResidentObjectListHead)
                {
                    ManagedObject* pObject = CONTAINING_RECORD(pResourceEntry, ManagedObject, m_ListEntry);

                    if (SyncPoint < pObject->m_LastGPUSyncPoint || 
                        CurrentUsage < CurrentBudget)
                    {
                        break;
                    }

                    ASSERT(pObject->m_eResidencyStatus == ManagedObject::RESIDENCY_STATUS::RESIDENT);

                    EvictionList[NumObjectsToEvict++] = pObject->m_pUnderlyingD3D12Pageable;
                    MakeStatusEvict(pObject);

                    CurrentUsage -= pObject->m_PageableBytesSize;

                    pResourceEntry = m_ResidentObjectListHead.Flink;
                }
            }

            // Trim all objects which are older than the specified time
            void TrimAgedAllocations(DeviceWideSyncPoint* MaxSyncPoint, ID3D12Pageable** EvictionList, uint32_t& NumObjectsToEvict, uint64_t CurrentTimeStamp, uint64_t MinDelta)
            {
                LIST_ENTRY* pResourceEntry = m_ResidentObjectListHead.Flink;

                while (pResourceEntry != &m_ResidentObjectListHead)
                {
                    ManagedObject* pObject = CONTAINING_RECORD(pResourceEntry, ManagedObject, m_ListEntry);

                    if ((MaxSyncPoint && MaxSyncPoint->GenerationID <= pObject->m_LastGPUSyncPoint) || // Only trim allocations done on the GPU
                        CurrentTimeStamp - pObject->m_LastUsedTimestamp <= MinDelta) // Don't evict things which have been used recently
                    {
                        break;
                    }

                    ASSERT(pObject->m_eResidencyStatus == ManagedObject::RESIDENCY_STATUS::RESIDENT);
                    EvictionList[NumObjectsToEvict++] = pObject->m_pUnderlyingD3D12Pageable;
                    MakeStatusEvict(pObject);

                    pResourceEntry = m_ResidentObjectListHead.Flink;
                }
            }

            ManagedObject* GetResidentListHead()
            {
                if (listHelper::IsListEmpty(&m_ResidentObjectListHead))
                {
                    return nullptr;
                }
                return CONTAINING_RECORD(m_ResidentObjectListHead.Flink, ManagedObject, m_ListEntry);
            }

            LIST_ENTRY m_ResidentObjectListHead;
            LIST_ENTRY m_EvictedObjectListHead;

            uint32_t m_NumResidentObjects;
            uint32_t m_NumEvictedObjects;

            uint64_t m_ResidentSize;
        }; // LRUCache

        class ResidencyManagerInternal
        {
        public:
            explicit ResidencyManagerInternal(SyncManager* pSyncManagerIn) :
                m_pParentDevice(nullptr),
                m_pDevice3(nullptr),
#ifdef __ID3D12DeviceDownlevel_INTERFACE_DEFINED__
                DeviceDownlevel(nullptr),
#endif
                m_AsyncThreadFence(0),
                m_hCompletionEvent(INVALID_HANDLE_VALUE),
                m_hAsyncThreadWorkCompletionEvent(INVALID_HANDLE_VALUE),
                m_pIDXGIAdapter(nullptr),
                m_hAsyncWorkEvent(INVALID_HANDLE_VALUE),
                m_PagingWorkThread(INVALID_HANDLE_VALUE),
                m_bFinishAsyncWork(false),
                m_bkStartEvicted(false),
                m_CurrentSyncPointGeneration(0),
                m_NumQueuesSeen(0),
                m_NodeIndex(0),
                m_CurrentAsyncWorkloadHead(0),
                m_CurrentAsyncWorkloadTail(0),
                m_kMinEvictionGracePeriod(1.0f),
                m_kMaxEvictionGracePeriod(60.0f),
                m_MinEvictionGracePeriodTicks(0ull), // In Init Initialize()
                m_MaxEvictionGracePeriodTicks(0ull), // In Init Initialize()
                m_kTrimPercentageMemoryUsageThreshold(0.7f),
                m_pAsyncPagingWorkQueue(nullptr),
                m_MaxSoftwareQueueLatency(6),
                m_AsyncWorkQueueSize(7),
                m_pSyncManager(pSyncManagerIn)
            {
                listHelper::InitializeListHead(&m_QueueFencesListHead);
                listHelper::InitializeListHead(&m_InFlightSyncPointsListHead);

                BOOL LuidSuccess = AllocateLocallyUniqueId(&m_ResidencyManagerUniqueID);
                ASSERT(LuidSuccess);
                UNREFERENCED_PARAMETER(LuidSuccess);
            };

            // NOTE: DeviceNodeIndex is an index not a mask. The majority of D3D12 uses bit masks to identify a GPU node whereas DXGI uses 0 based indices.
            HRESULT Initialize(IDXGIAdapter* pParentAdapter, ID3D12Device* pParentDevice, UINT deviceNodeIndex,  uint32_t maxSoftwareQueueLatency)
            {
                m_pParentDevice           = pParentDevice;
                m_NodeIndex               = deviceNodeIndex;
                m_MaxSoftwareQueueLatency = maxSoftwareQueueLatency;

                // Try to query for the device interface with a queued MakeResident API.
                if (FAILED(m_pParentDevice->QueryInterface(&m_pDevice3)))
                {
                    // The queued MakeResident API is not available. Start the paging fence at 1.
                    // AsyncThreadFence.Increment();
                    ::InterlockedIncrement(&m_AsyncThreadFence.FenceValue);
                }

#ifdef __ID3D12DeviceDownlevel_INTERFACE_DEFINED__
                Device->QueryInterface(&DeviceDownlevel);
#endif

                if (pParentAdapter)
                {
                    pParentAdapter->QueryInterface(&m_pIDXGIAdapter);
                }

                m_AsyncWorkQueueSize    = (size_t)(maxSoftwareQueueLatency + 1);
                m_pAsyncPagingWorkQueue = new AsyncPagingWork[m_AsyncWorkQueueSize];

                if (m_pAsyncPagingWorkQueue == nullptr)
                {
                    return E_OUTOFMEMORY;
                }

                LARGE_INTEGER Frequency;
                QueryPerformanceFrequency(&Frequency);

                // Calculate how many QPC ticks are equivalent to the given time in seconds
                m_MinEvictionGracePeriodTicks = uint64_t(Frequency.QuadPart * m_kMinEvictionGracePeriod);
                m_MaxEvictionGracePeriodTicks = uint64_t(Frequency.QuadPart * m_kMaxEvictionGracePeriod);

                HRESULT hr = S_OK;
                hr = m_AsyncThreadFence.Initialize(m_pParentDevice);

                if (SUCCEEDED(hr))
                {
                    m_hCompletionEvent = CreateEvent(nullptr, false, false, nullptr);
                    if (m_hCompletionEvent == INVALID_HANDLE_VALUE)
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }

                if (SUCCEEDED(hr))
                {
                    m_hAsyncThreadWorkCompletionEvent = CreateEvent(nullptr, false, false, nullptr);
                    if (m_hAsyncThreadWorkCompletionEvent == INVALID_HANDLE_VALUE)
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }

                if (SUCCEEDED(hr))
                {
                    m_hAsyncWorkEvent = CreateEvent(nullptr, true, false, nullptr);
                    if (m_hAsyncWorkEvent == INVALID_HANDLE_VALUE)
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }

#if !RESIDENCY_SINGLE_THREADED
                if (SUCCEEDED(hr) && !m_pDevice3)
                {
                    m_PagingWorkThread = CreateThread(nullptr, 0, AsyncThreadStart, (void*) this, 0, nullptr);

                    if (m_PagingWorkThread == INVALID_HANDLE_VALUE)
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
#endif

                return hr;
            }
            void Destroy()
            {
                m_AsyncThreadFence.Destroy();

                if (m_hCompletionEvent != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(m_hCompletionEvent);
                    m_hCompletionEvent = INVALID_HANDLE_VALUE;
                }

#if !RESIDENCY_SINGLE_THREADED
                AsyncPagingWork* pWork = DequeueAsyncWork();

                while (pWork)
                {
                    pWork = DequeueAsyncWork();
                }

                m_bFinishAsyncWork = true;

                if (SetEvent(m_hAsyncWorkEvent) == false)
                {
                    ASSERT_HR(HRESULT_FROM_WIN32(GetLastError()));
                }

                // Make sure the async worker thread is finished to prevent dereferencing
                // dangling pointers to ResidencyManagerInternal
                if (m_PagingWorkThread != INVALID_HANDLE_VALUE)
                {
                    WaitForSingleObject(m_PagingWorkThread, INFINITE);
                    CloseHandle(m_PagingWorkThread);
                    m_PagingWorkThread = INVALID_HANDLE_VALUE;
                }

                if (m_hAsyncWorkEvent != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(m_hAsyncWorkEvent);
                    m_hAsyncWorkEvent = INVALID_HANDLE_VALUE;
                }
#endif

                if (m_hAsyncThreadWorkCompletionEvent != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(m_hAsyncThreadWorkCompletionEvent);
                    m_hAsyncThreadWorkCompletionEvent = INVALID_HANDLE_VALUE;
                }

                while (listHelper::IsListEmpty(&m_QueueFencesListHead) == false)
                {
                    Internal::FenceStructure* pObject =
                        CONTAINING_RECORD(m_QueueFencesListHead.Flink, Internal::FenceStructure, ListEntry);

                    pObject->Destroy();
                    listHelper::RemoveHeadList(&m_QueueFencesListHead);
                    delete(pObject);
                }

                while (listHelper::IsListEmpty(&m_InFlightSyncPointsListHead) == false)
                {
                    Internal::DeviceWideSyncPoint* pPoint =
                        CONTAINING_RECORD(m_InFlightSyncPointsListHead.Flink, Internal::DeviceWideSyncPoint, ListEntry);

                    listHelper::RemoveHeadList(&m_InFlightSyncPointsListHead);
                    delete pPoint;
                }

                delete [] m_pAsyncPagingWorkQueue;

                if (m_pDevice3)
                {
                    m_pDevice3->Release();
                    m_pDevice3 = nullptr;
                }

#ifdef __ID3D12DeviceDownlevel_INTERFACE_DEFINED__
                if (DeviceDownlevel)
                {
                    DeviceDownlevel->Release();
                    DeviceDownlevel = nullptr;
                }
#endif

                if (m_pIDXGIAdapter)
                {
                    m_pIDXGIAdapter->Release();
                    m_pIDXGIAdapter = nullptr;
                }
            }
            void BeginTrackingObject(ManagedObject* pObject)
            {
                Internal::ScopedLock Lock(&m_CS);

                if (pObject)
                {
                    ASSERT(pObject->m_pUnderlyingD3D12Pageable != nullptr);
                    if (m_bkStartEvicted)
                    {
                        pObject->m_eResidencyStatus = ManagedObject::RESIDENCY_STATUS::EVICTED;
                        ASSERT_HR(m_pParentDevice->Evict(1, &pObject->m_pUnderlyingD3D12Pageable));
                    }

                    m_InternalLRUCache.InsertFromResidentListHead(pObject);
                }
            }
            void EndTrackingObject(ManagedObject* pObject)
            {
                Internal::ScopedLock Lock(&m_CS);

                m_InternalLRUCache.RemoveFromResidentListHead(pObject);
            }

            // One residency set per command-list
            HRESULT ExecuteCommandLists(ID3D12CommandQueue* pQueue, ID3D12CommandList** pCommandLists, ResidencySet** pResidencySets, uint32_t numResidencySets)
            {
                return ExecuteSubsetRecursive(pQueue, pCommandLists, pResidencySets, numResidencySets, 0u);
            }
            HRESULT GetCurrentGPUSyncPoint(ID3D12CommandQueue* Queue, uint64_t *pGPUSyncPoint)
            {
                Internal::FenceStructure* QueueFence = nullptr;
                HRESULT hr = GetFence(Queue, QueueFence);

                // The signal and increment need to be atomic
                if(SUCCEEDED(hr))
                {
                    Internal::ScopedLock Lock(&m_ExecutionCS);
                    *pGPUSyncPoint = QueueFence->FenceValue;
                    hr = SignalFence(Queue, QueueFence);
                }

                ASSERT_HR(hr);

                return hr;
            }

        private:
            HRESULT GetFence(ID3D12CommandQueue *Queue, Internal::FenceStructure *&QueueFence)
            {
                // We have to track each object on each queue so we know when it is safe to evict them. 
                // Therefore, for every queue that we see, associate a fence with it
                GUID FenceGuid = { 0xf0, 0, 0xd, { 0, 0, 0, 0, 0, 0, 0, 0 } };
                memcpy(&FenceGuid.Data4, &m_ResidencyManagerUniqueID, sizeof(m_ResidencyManagerUniqueID));

                QueueFence = nullptr;
                HRESULT hr = S_OK;

                struct
                {
                    Internal::FenceStructure* pFence;
                } CommandQueuePrivateData;

                // Find or create the fence for this queue
                {
                    uint32_t Size = sizeof(CommandQueuePrivateData);
                    hr = Queue->GetPrivateData(FenceGuid, &Size, &CommandQueuePrivateData);
                    if (FAILED(hr))
                    {
                        QueueFence = new Internal::FenceStructure(1);
                        hr = QueueFence->Initialize(m_pParentDevice);
                        listHelper::InsertTailList(&m_QueueFencesListHead, &QueueFence->ListEntry);

                        InterlockedIncrement(&m_NumQueuesSeen);

                        if (SUCCEEDED(hr))
                        {
                            CommandQueuePrivateData = { QueueFence };
                            hr = Queue->SetPrivateData(FenceGuid, uint32_t(sizeof(CommandQueuePrivateData)), &CommandQueuePrivateData);
                            ASSERT_HR(hr);
                        }
                    }
                    QueueFence = CommandQueuePrivateData.pFence;
                    ASSERT(QueueFence != nullptr);
                }

                // ASSERT_HR(hr);

                return hr;
            }
            HRESULT SignalFence(ID3D12CommandQueue *Queue, Internal::FenceStructure *QueueFence)
            {
                // When this fence is passed it is safe to evict the resources used in the list just submitted
                HRESULT hr = QueueFence->GPUSignal(Queue);

                // QueueFence->Increment();
                ::InterlockedIncrement(&QueueFence->FenceValue);

                if (SUCCEEDED(hr))
                {
                    hr = EnqueueSyncPoint();
                    ASSERT_HR(hr);
                }

                m_CurrentSyncPointGeneration++;

                // ASSERT_HR(hr);

                return hr;
            }

            HRESULT ExecuteSubsetRecursive(ID3D12CommandQueue* pQueue, ID3D12CommandList** pCommandLists, ResidencySet** pResidencySets, uint32_t numResidencySets, uint32_t debugRecursiveDepth)
            {
                HRESULT HardwareResult = S_OK;

                DXGI_QUERY_VIDEO_MEMORY_INFO LocalMemory;
                ZeroMemory(&LocalMemory, sizeof(LocalMemory));
                GetCurrentBudget(&LocalMemory, DXGI_MEMORY_SEGMENT_GROUP_LOCAL);

                DXGI_QUERY_VIDEO_MEMORY_INFO NonLocalMemory;
                ZeroMemory(&NonLocalMemory, sizeof(NonLocalMemory));
                GetCurrentBudget(&NonLocalMemory, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL);

                uint64_t TotalSizeNeeded = 0;
                uint32_t MaxObjectsReferenced = 0;

                for (uint32_t i = 0; i < numResidencySets; ++i)
                {
                    if (pResidencySets[i])
                    {
                        // if (pResidencySets[i]->m_bOpen)
                        // {
                            // Residency Sets must be closed before execution just like Command Lists
                        //     return E_INVALIDARG;
                        // }

                        ASSERT(pResidencySets[i]->m_bOpen);

                        MaxObjectsReferenced += pResidencySets[i]->m_CurrentSetSize;
                    }
                }

                // Create a set to gather up all unique resources required by this call
                ResidencySet* pMasterSet = new ResidencySet();
                if (pMasterSet == nullptr || pMasterSet->InitializeResidencySet(m_pSyncManager, MaxObjectsReferenced) == false)
                {
                    ASSERT(false);
                    return E_OUTOFMEMORY;
                }

                HardwareResult = pMasterSet->Open();
                if (FAILED(HardwareResult))
                {
                    // ASSERT_HR(HardwareResult);
                    return HardwareResult;
                }

                // For each residency set
                for (uint32_t i = 0; i < numResidencySets; ++i)
                {
                    if (pResidencySets[i])
                    {
                        // For each object in this set
                        for (int x = 0; x < pResidencySets[i]->m_CurrentSetSize; ++x)
                        {
                            if (pMasterSet->Insert(pResidencySets[i]->m_ppManagedObjectSet[x]))
                            {
                                TotalSizeNeeded += pResidencySets[i]->m_ppManagedObjectSet[x]->m_PageableBytesSize;
                            }
                        }
                    }
                }

                // Close this set to free it's slot up for the app
                HardwareResult = pMasterSet->Close();
                // ASSERT(HardwareResult);
                if (FAILED(HardwareResult))
                {
                    return HardwareResult;
                }

                // This set of commandlists can't possibly fit within the budget, they need to be split up. 
                // If the number of command lists is 1 there is nothing we can do
                if (numResidencySets && (LocalMemory.Budget + NonLocalMemory.Budget < TotalSizeNeeded))
                {
                    delete(pMasterSet);

                    // Recursively try to find a small enough set to fit in memory
                    const uint32_t Half   = numResidencySets / 2;
                    const HRESULT LowerHR = ExecuteSubsetRecursive(pQueue, pCommandLists, pResidencySets, Half, debugRecursiveDepth + 1);
                    const HRESULT UpperHR = ExecuteSubsetRecursive(pQueue, &pCommandLists[Half], &pResidencySets[Half], numResidencySets - Half, debugRecursiveDepth + 1);

                    return (LowerHR == S_OK && UpperHR == S_OK) ? S_OK : E_FAIL;
                }

                Internal::FenceStructure* pQueueFence = nullptr;
                HardwareResult = GetFence(pQueue, pQueueFence);

                ASSERT_HR(HardwareResult);

                if (SUCCEEDED(HardwareResult))
                {
                    // The following code must be atomic so that things get ordered correctly

                    Internal::ScopedLock Lock(&m_ExecutionCS);

                    // Evict or make resident all of the objects we identified above.
                    // This will run on an async thread, allowing the current to continue while still blocking the GPU if required
                    // If a native async MakeResident is supported, this will run on this thread - it will only block until work referencing
                    // resources which need to be evicted is completed, and does not need to wait for MakeResident to complete.
                    HardwareResult = EnqueueAsyncWork(pMasterSet, m_AsyncThreadFence.FenceValue, m_CurrentSyncPointGeneration);

#if !RESIDENCY_SINGLE_THREADED
                    if (m_pDevice3)
#endif
                    {
                        AsyncPagingWork* pWorkload = DequeueAsyncWork();
                        ProcessPagingWork(pWorkload);
                    }

                    // If there are some things that need to be made resident we need to make sure that the GPU
                    // doesn't execute until the async thread signals that the MakeResident call has returned.
                    if (SUCCEEDED(HardwareResult))
                    {
                        HardwareResult = m_AsyncThreadFence.GPUWait(pQueue);

                        // If we're using a queued MakeResident, then ProcessPagingWork may increment the fence multiple times instead of
                        // signaling a pre-defined value.
                        if (!m_pDevice3)
                        {
                            // AsyncThreadFence.Increment();
                            ::InterlockedIncrement(&m_AsyncThreadFence.FenceValue);
                        }
                    }

                    pQueue->ExecuteCommandLists(numResidencySets, pCommandLists);

                    if (SUCCEEDED(HardwareResult))
                    {
                        HardwareResult = SignalFence(pQueue, pQueueFence);
                    }
                }
                return HardwareResult;
            }

            // InternalResidency::AsyncWorkQueue[]
            struct AsyncPagingWork
            {
                explicit AsyncPagingWork() 
                    :
                    pMasterSet(nullptr),
                    FenceValueToSignal(0),
                    SyncPointGeneration(0)
                {
                }

                uint64_t SyncPointGeneration;

                // List of objects to make resident
                ResidencySet* pMasterSet;

                // The GPU will wait on this value so that it doesn't execute until the objects are made resident
                uint64_t FenceValueToSignal;
            };

            static unsigned long WINAPI AsyncThreadStart(void* pData)
            {
                ResidencyManagerInternal* pManager = (ResidencyManagerInternal*)pData;

                while (1)
                {
                    AsyncPagingWork* pWork = pManager->DequeueAsyncWork();

                    while (pWork)
                    {
                        // Submit the work
                        pManager->ProcessPagingWork(pWork);
                        if (SetEvent(pManager->m_hAsyncThreadWorkCompletionEvent) == false)
                        {
                            ASSERT_HR(HRESULT_FROM_WIN32(GetLastError()));
                        }

                        // Get more work
                        pWork = pManager->DequeueAsyncWork();
                    }

                    //Wait until there is more work do be done
                    WaitForSingleObject(pManager->m_hAsyncWorkEvent, INFINITE);

                    if (ResetEvent(pManager->m_hAsyncWorkEvent) == false)
                    {
                        ASSERT_HR(HRESULT_FROM_WIN32(GetLastError()));
                    }

                    if (pManager->m_bFinishAsyncWork)
                    {
                        return 0;
                    }
                }

                return 0;
            }

            // This will be run from a worker thread and will emulate a software queue for making gpu resources resident or evicted.
            // The GPU will be synchronized by this queue to ensure that it never executes using an evicted resource.
            void ProcessPagingWork(AsyncPagingWork* pWork)
            {
                Internal::DeviceWideSyncPoint* FirstUncompletedSyncPoint = DequeueCompletedSyncPoints();

                // Use a union so that we only need 1 allocation
                union ResidentScratchSpace
                {
                    ManagedObject* pManagedObject;
                    ID3D12Pageable* pUnderlying;
                };

                ResidentScratchSpace* pMakeResidentList = nullptr;
                uint32_t NumObjectsToMakeResident = 0;

                ID3D12Pageable** pEvictionList = nullptr;
                uint32_t NumObjectsToEvict = 0;

                // the size of all the objects which will need to be made resident in order to execute this set.
                uint64_t SizeToMakeResident = 0;

                LARGE_INTEGER CurrentTime = {};
                QueryPerformanceCounter(&CurrentTime);

                {
                    // A lock must be taken here as the state of the objects will be altered
                    Internal::ScopedLock Lock(&m_CS);

                    pMakeResidentList = new ResidentScratchSpace[pWork->pMasterSet->m_CurrentSetSize];
                    pEvictionList     = new ID3D12Pageable*[m_InternalLRUCache.m_NumResidentObjects];

                    // Mark the objects used by this command list to be made resident
                    for (int i = 0; i < pWork->pMasterSet->m_CurrentSetSize; ++i)
                    {
                        ManagedObject*& pObject = pWork->pMasterSet->m_ppManagedObjectSet[i];
                        // If it's evicted we need to make it resident again
                        if (pObject->m_eResidencyStatus == ManagedObject::RESIDENCY_STATUS::EVICTED)
                        {
                            pMakeResidentList[NumObjectsToMakeResident++].pManagedObject = pObject;
                            m_InternalLRUCache.MakeStatusResident(pObject);

                            SizeToMakeResident += pObject->m_PageableBytesSize;
                        }

                        // Update the last sync point that this was used on
                        pObject->m_LastGPUSyncPoint = pWork->SyncPointGeneration;

                        pObject->m_LastUsedTimestamp = CurrentTime.QuadPart;
                        m_InternalLRUCache.ObjectReferenced(pObject);
                    }

                    DXGI_QUERY_VIDEO_MEMORY_INFO LocalMemory = {};
                    ZeroMemory(&LocalMemory, sizeof(LocalMemory));
                    GetCurrentBudget(&LocalMemory, DXGI_MEMORY_SEGMENT_GROUP_LOCAL);

                    uint64_t EvictionGracePeriod = GetCurrentEvictionGracePeriod(&LocalMemory);
                    m_InternalLRUCache.TrimAgedAllocations(FirstUncompletedSyncPoint, pEvictionList, NumObjectsToEvict, CurrentTime.QuadPart, EvictionGracePeriod);

                    if (NumObjectsToEvict)
                    {
                        ASSERT_HR(m_pParentDevice->Evict(NumObjectsToEvict, pEvictionList));
                        NumObjectsToEvict = 0;
                    }

                    if (NumObjectsToMakeResident)
                    {
                        uint32_t ObjectsMadeResident = 0;
                        uint32_t MakeResidentIndex = 0;
                        while (true)
                        {
                            ZeroMemory(&LocalMemory, sizeof(LocalMemory));

                            GetCurrentBudget(&LocalMemory, DXGI_MEMORY_SEGMENT_GROUP_LOCAL);
                            DXGI_QUERY_VIDEO_MEMORY_INFO NonLocalMemory;
                            ZeroMemory(&NonLocalMemory, sizeof(NonLocalMemory));
                            GetCurrentBudget(&NonLocalMemory, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL);

                            int64_t TotalUsage = LocalMemory.CurrentUsage + NonLocalMemory.CurrentUsage;
                            int64_t TotalBudget = LocalMemory.Budget + NonLocalMemory.Budget;

                            int64_t AvailableSpace = TotalBudget - TotalUsage;

                            uint64_t BatchSize = 0;
                            uint32_t NumObjectsInBatch = 0;
                            uint32_t BatchStart = MakeResidentIndex;

                            HRESULT ResidentResult = S_OK;

                            if (0 < AvailableSpace)
                            {
                                for (uint32_t i = MakeResidentIndex; i < NumObjectsToMakeResident; ++i)
                                {
                                    // If we try to make this object resident, will we go over budget?
                                    if (uint64_t(AvailableSpace) < BatchSize + pMakeResidentList[i].pManagedObject->m_PageableBytesSize)
                                    {
                                        // Next time we will start here
                                        MakeResidentIndex = i;
                                        break;
                                    }
                                    else
                                    {
                                        BatchSize += pMakeResidentList[i].pManagedObject->m_PageableBytesSize;
                                        NumObjectsInBatch++;
                                        ObjectsMadeResident++;

                                        pMakeResidentList[i].pUnderlying = pMakeResidentList[i].pManagedObject->m_pUnderlyingD3D12Pageable;
                                    }
                                }

                                if (m_pDevice3)
                                {
                                    ResidentResult = m_pDevice3->EnqueueMakeResident(D3D12_RESIDENCY_FLAG_NONE,
                                                                      NumObjectsInBatch,
                                                                      &pMakeResidentList[BatchStart].pUnderlying,
                                                                      m_AsyncThreadFence.pFence,
                                                                      m_AsyncThreadFence.FenceValue + 1);
                                    if (SUCCEEDED(ResidentResult))
                                    {
                                        // AsyncThreadFence.Increment();
                                        ::InterlockedIncrement(&m_AsyncThreadFence.FenceValue);
                                    }
                                }
                                else
                                {
                                    ResidentResult = m_pParentDevice->MakeResident(NumObjectsInBatch, &pMakeResidentList[BatchStart].pUnderlying);
                                }
                                if (SUCCEEDED(ResidentResult))
                                {
                                    SizeToMakeResident -= BatchSize;
                                }
                            }

                            if (FAILED(ResidentResult) || ObjectsMadeResident != NumObjectsToMakeResident)
                            {
                                ManagedObject* pResidentHead = m_InternalLRUCache.GetResidentListHead();

                                // Get the next sync point to wait for
                                FirstUncompletedSyncPoint = DequeueCompletedSyncPoints();

                                // If there is nothing to trim OR the only objects 'Resident' are the ones about to be used by this execute.
								if (
									pResidentHead == nullptr ||
									pWork->SyncPointGeneration <= pResidentHead->m_LastGPUSyncPoint ||
									FirstUncompletedSyncPoint == nullptr
									)
                                {
                                    // Make resident the rest of the objects as there is nothing left to trim
                                    uint32_t NumObjects = NumObjectsToMakeResident - ObjectsMadeResident;

                                    // Gather up the remaining underlying objects
                                    for (uint32_t i = MakeResidentIndex; i < NumObjectsToMakeResident; i++)
                                    {
                                        pMakeResidentList[i].pUnderlying = pMakeResidentList[i].pManagedObject->m_pUnderlyingD3D12Pageable;
                                    }

                                    if (m_pDevice3)
                                    {
                                        ResidentResult = m_pDevice3->EnqueueMakeResident(D3D12_RESIDENCY_FLAG_NONE,
                                                                          NumObjectsInBatch,
                                                                          &pMakeResidentList[BatchStart].pUnderlying,
                                                                          m_AsyncThreadFence.pFence,
                                                                          m_AsyncThreadFence.FenceValue + 1);
                                        if (SUCCEEDED(ResidentResult))
                                        {
                                            // AsyncThreadFence.Increment();
                                            ::InterlockedIncrement(&m_AsyncThreadFence.FenceValue);
                                        }
                                    }
                                    else
                                    {
                                        ResidentResult = m_pParentDevice->MakeResident(NumObjects, &pMakeResidentList[MakeResidentIndex].pUnderlying);
                                    }
                                    
                                    // TODO: What should we do if this fails? This is a catastrophic failure in which the app is trying to use more memory
                                    //       in 1 command list than can possibly be made resident by the system.
                                    ASSERT_HR(ResidentResult);
                                    break;
                                }

                                uint64_t GenerationToWaitFor = FirstUncompletedSyncPoint->GenerationID;

                                // We can't wait for the sync-point that this work is intended for
                                if (GenerationToWaitFor == pWork->SyncPointGeneration)
                                {
                                    --GenerationToWaitFor;
                                }
                                // Wait until the GPU is done
                                WaitForSyncPoint(GenerationToWaitFor);

                                m_InternalLRUCache.TrimToSyncPointInclusive
                                (
                                    TotalUsage + int64_t(SizeToMakeResident), TotalBudget, 
                                    pEvictionList, NumObjectsToEvict, 
                                    GenerationToWaitFor
                                );

                                ASSERT_HR(m_pParentDevice->Evict(NumObjectsToEvict, pEvictionList));
                            }
                            else
                            {
                                // We made everything resident, mission accomplished
                                break;
                            }
                        }
                    }

                    delete[](pMakeResidentList);
                    delete[](pEvictionList);
                }

                if (!m_pDevice3)
                {
                    // Tell the GPU that it's safe to execute since we made things resident
                    ASSERT_HR(m_AsyncThreadFence.pFence->Signal(pWork->FenceValueToSignal));
                }

                delete(pWork->pMasterSet);
                pWork->pMasterSet = nullptr;
            }

            // The Enqueue and Dequeue Async Work functions are threadsafe as there is only 1 producer and 1 consumer, 
            // if that changes Synchronisation will be required
            HRESULT EnqueueAsyncWork(ResidencySet* pMasterSet, uint64_t FenceValueToSignal, uint64_t SyncPointGeneration)
            {
                // We can't get too far ahead of the worker thread otherwise huge hitches occur
                while (m_MaxSoftwareQueueLatency <= (m_CurrentAsyncWorkloadTail - m_CurrentAsyncWorkloadHead))
                {
                    WaitForSingleObject(m_hAsyncThreadWorkCompletionEvent, INFINITE);
                }

                ASSERT(m_CurrentAsyncWorkloadHead <= m_CurrentAsyncWorkloadTail);

                const size_t currentIndex                                 = m_CurrentAsyncWorkloadTail % m_AsyncWorkQueueSize;
                m_pAsyncPagingWorkQueue[currentIndex].pMasterSet          = pMasterSet;
                m_pAsyncPagingWorkQueue[currentIndex].FenceValueToSignal  = FenceValueToSignal;
                m_pAsyncPagingWorkQueue[currentIndex].SyncPointGeneration = SyncPointGeneration;

                m_CurrentAsyncWorkloadTail++;

                if (SetEvent(m_hAsyncWorkEvent) == false)
                {
                    return HRESULT_FROM_WIN32(GetLastError());
                }

                return S_OK;
            }

            AsyncPagingWork* DequeueAsyncWork()
            {
                if (m_CurrentAsyncWorkloadHead == m_CurrentAsyncWorkloadTail)
                {
                    return nullptr;
                }

                const size_t currentHead = m_CurrentAsyncWorkloadHead % m_AsyncWorkQueueSize;
                AsyncPagingWork* pWork = &m_pAsyncPagingWorkQueue[currentHead];

                m_CurrentAsyncWorkloadHead++;
                return pWork;
            }

            HRESULT EnqueueSyncPoint()
            {
                Internal::ScopedLock Lock(&m_AsyncWorkCS);

                Internal::DeviceWideSyncPoint* pPoint = Internal::DeviceWideSyncPoint::CreateSyncPoint(m_NumQueuesSeen, m_CurrentSyncPointGeneration);
                if (pPoint == nullptr)
                {
                    return E_OUTOFMEMORY;
                }

                uint32_t i = 0;
                LIST_ENTRY* pFenceEntry = m_QueueFencesListHead.Flink;
                // Record the current state of each queue we track into this sync point
                while (pFenceEntry != &m_QueueFencesListHead)
                {
                    Internal::FenceStructure* pFence = CONTAINING_RECORD(pFenceEntry, Internal::FenceStructure, ListEntry);
                    pFenceEntry = pFenceEntry->Flink;

                    pPoint->pQueueSyncPoints[i].pFence = pFence;
                    pPoint->pQueueSyncPoints[i].LastUsedValue = pFence->FenceValue - 1;//Minus one as we want the last submitted

                    i++;
                }

                listHelper::InsertTailList(&m_InFlightSyncPointsListHead, &pPoint->ListEntry);

                return S_OK;
            }

            // Returns a pointer to the first synch point which is not completed
            Internal::DeviceWideSyncPoint* DequeueCompletedSyncPoints()
            {
                Internal::ScopedLock Lock(&m_AsyncWorkCS);

                while (listHelper::IsListEmpty(&m_InFlightSyncPointsListHead) == false)
                {
                    Internal::DeviceWideSyncPoint* pPoint =
                        CONTAINING_RECORD(m_InFlightSyncPointsListHead.Flink, Internal::DeviceWideSyncPoint, ListEntry);

                    if (pPoint->IsCompleted())
                    {
                        listHelper::RemoveHeadList(&m_InFlightSyncPointsListHead);
                        delete pPoint;
                    }
                    else
                    {
                        return pPoint;
                    }
                }

                return nullptr;
            }

            void WaitForSyncPoint(uint64_t SyncPointID)
            {
                Internal::ScopedLock Lock(&m_AsyncWorkCS);

                LIST_ENTRY* pPointEntry = m_InFlightSyncPointsListHead.Flink;
                while (pPointEntry != &m_InFlightSyncPointsListHead)
                {
                    Internal::DeviceWideSyncPoint* pPoint =
                        CONTAINING_RECORD(m_InFlightSyncPointsListHead.Flink, Internal::DeviceWideSyncPoint, ListEntry);

                    if (pPoint->GenerationID > SyncPointID)
                    {
                        // this point is already done
                        return;
                    }
                    else if (pPoint->GenerationID < SyncPointID)
                    {
                        // Keep popping off until we find the one to wait on
                        listHelper::RemoveHeadList(&m_InFlightSyncPointsListHead);
                        delete(pPoint);
                    }
                    else
                    {
                        pPoint->WaitForCompletion(m_hCompletionEvent);
                        listHelper::RemoveHeadList(&m_InFlightSyncPointsListHead);
                        delete(pPoint);
                        return;
                    }
                }
            }

            void GetCurrentBudget(DXGI_QUERY_VIDEO_MEMORY_INFO* InfoOut, DXGI_MEMORY_SEGMENT_GROUP Segment)
            {
                if (m_pIDXGIAdapter)
                {
                    ASSERT_HR(m_pIDXGIAdapter->QueryVideoMemoryInfo(m_NodeIndex, Segment, InfoOut));
                }
#ifdef __ID3D12DeviceDownlevel_INTERFACE_DEFINED__
                else if (DeviceDownlevel)
                {
                    ASSERT_HR(DeviceDownlevel->QueryVideoMemoryInfo(NodeIndex, Segment, InfoOut));
                }
#endif
            }
            // Generate a result between the minimum period and the maximum period based on the current local memory pressure. 
            // I.e. when memory pressure is low, objects will persist longer before being evicted.
            uint64_t GetCurrentEvictionGracePeriod(DXGI_QUERY_VIDEO_MEMORY_INFO* LocalMemoryState)
            {
                // 1 == full pressure, 0 == no pressure
                double Pressure = (double(LocalMemoryState->CurrentUsage) / double(LocalMemoryState->Budget));
                Pressure = (Pressure < 1.0) ? Pressure : 1.0f;

                if (m_kTrimPercentageMemoryUsageThreshold < Pressure)
                {
                    // Normalize the pressure for the range 0 to cTrimPercentageMemoryUsageThreshold
                    Pressure = (Pressure - m_kTrimPercentageMemoryUsageThreshold) / (1.0 - m_kTrimPercentageMemoryUsageThreshold);

                    // Linearly interpolate between the min period and the max period based on the pressure
                    return uint64_t((m_MaxEvictionGracePeriodTicks - m_MinEvictionGracePeriodTicks) * (1.0 - Pressure)) + m_MinEvictionGracePeriodTicks;
                }
                else
                {
                    // Essentially don't trim at all
                    return MAXUINT64;
                }
            }

		private:
			size_t           m_AsyncWorkQueueSize;
			AsyncPagingWork* m_pAsyncPagingWorkQueue;

			HANDLE m_hAsyncWorkEvent;
			HANDLE m_PagingWorkThread;
			Internal::CriticalSection m_AsyncWorkCS;

			volatile bool   m_bFinishAsyncWork;
			volatile size_t m_CurrentAsyncWorkloadHead;
			volatile size_t m_CurrentAsyncWorkloadTail;

			LIST_ENTRY               m_QueueFencesListHead;
            uint32_t                 m_NumQueuesSeen;
            Internal::FenceStructure m_AsyncThreadFence;

            LIST_ENTRY m_InFlightSyncPointsListHead;
            uint64_t   m_CurrentSyncPointGeneration;

            HANDLE m_hCompletionEvent;
            HANDLE m_hAsyncThreadWorkCompletionEvent;
            Internal::CriticalSection m_CS;
            Internal::CriticalSection m_ExecutionCS;

            // NOTE: This is an index not a mask.
            UINT           m_NodeIndex;
            IDXGIAdapter3* m_pIDXGIAdapter;

            ID3D12Device*  m_pParentDevice;
            ID3D12Device3* m_pDevice3;
#ifdef __ID3D12DeviceDownlevel_INTERFACE_DEFINED__
            ID3D12DeviceDownlevel* DeviceDownlevel;
#endif
            Internal::LRUCache m_InternalLRUCache;

            const bool  m_bkStartEvicted;
            const float m_kMinEvictionGracePeriod;
            const float m_kMaxEvictionGracePeriod;
            const float m_kTrimPercentageMemoryUsageThreshold; // (valid between 0.0 - 1.0)

            uint64_t m_MinEvictionGracePeriodTicks;
            uint64_t m_MaxEvictionGracePeriodTicks;

            uint32_t m_MaxSoftwareQueueLatency;
            LUID     m_ResidencyManagerUniqueID;

            SyncManager* m_pSyncManager;
        }; // class ResidencyManagerInternal
    }

    class ResidencyManager
    {
    public:
        ResidencyManager() 
            :
            Manager(&SyncManager),
            bInitialized(false)
        {
        }

        // NOTE: DeviceNodeIndex is an index not a mask. 
        // The majority of D3D12 uses bit masks to identify a GPU node whereas DXGI uses 0 based indices.
        FORCEINLINE HRESULT Initialize(IDXGIAdapter* pParentAdapter, ID3D12Device* pParentDevice, UINT deviceNodeIndex,  uint32_t maxSoftwareQueueLatency)
        {
            HRESULT hardwareResult = Manager.Initialize(pParentAdapter, pParentDevice, deviceNodeIndex, maxSoftwareQueueLatency);
            ASSERT_HR(hardwareResult);

            bInitialized = true;

            return hardwareResult;
        }

        FORCEINLINE void Destroy()
        {
            Manager.Destroy();
        }

        FORCEINLINE void BeginTrackingObject(ManagedObject* pObject)
        {
            Manager.BeginTrackingObject(pObject);
#if defined(_DEBUG)
            ++Debug_NumCurrentTraccingObjects;
#endif
        }

        FORCEINLINE void EndTrackingObject(ManagedObject* pObject)
        {
            Manager.EndTrackingObject(pObject);
#if defined(_DEBUG)
            --Debug_NumCurrentTraccingObjects;
#endif
        }

        HRESULT GetCurrentGPUSyncPoint(ID3D12CommandQueue* pD3D12CommandQueue, uint64_t *pCurrentGPUSyncPoint)
        {
            HRESULT hardwareResult = Manager.GetCurrentGPUSyncPoint(pD3D12CommandQueue, pCurrentGPUSyncPoint);
            ASSERT_HR(hardwareResult);
            return hardwareResult;
        }

        // One residency set per command-list
        FORCEINLINE HRESULT ExecuteCommandLists(ID3D12CommandQueue* pD3D12CommandQueue, ID3D12CommandList** p_pCommandLists, ResidencySet** p_pResidencySets, uint32_t numResidencySets)
        {
            HRESULT hardwareResult = Manager.ExecuteCommandLists(pD3D12CommandQueue, p_pCommandLists, p_pResidencySets, numResidencySets);
            ASSERT_HR(hardwareResult);
            return hardwareResult;
        }

        // TODO 0 : Optimization.
        FORCEINLINE ResidencySet* CreateResidencySet()
        {
            ASSERT(bInitialized);
            // TODO 0 : Optimization.
            ResidencySet* pSet = new ResidencySet();

            if (pSet)
            {
                pSet->InitializeResidencySet(&SyncManager);
            }
#if defined(_DEBUG)
            ++Debug_NumResidencySets;
#endif

            return pSet;
        }

        FORCEINLINE void DestroyResidencySet(ResidencySet* pSet)
        {
            ASSERT(bInitialized);

            delete(pSet);
#if defined(_DEBUG)
            --Debug_NumResidencySets;
#endif
        }

    private:
        Internal::ResidencyManagerInternal Manager;
        Internal::SyncManager              SyncManager;

        bool bInitialized;

#if defined(_DEBUG)
        size_t Debug_NumResidencySets          = 0ul;
        size_t Debug_NumCurrentTraccingObjects = 0ul;
#endif
    };
};
