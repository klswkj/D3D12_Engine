#include "stdafx.h"
#include "GPUWorkManager.h"

namespace custom
{
	TaskFiber::TaskFiber(CustomCommandQALPool* pPool, ID3D12CommandQueue* pCommandQueue, D3D12_COMMAND_LIST_TYPE type)
		:
		m_pOwningPool(pPool),
		m_Type(type),
		m_CustomFence(type)
	{
		ASSERT(pPool);
		ASSERT(pCommandQueue);
		ASSERT(CHECK_VALID_TYPE(type));

		m_pCommandQueue = pCommandQueue;
	}

	TaskFiber::~TaskFiber()
	{
		TaskFiberManager::Release(this);
	}

	bool TaskFiber::IsCompletedTask(uint64_t fenceValue /*= 0*/)
	{
		return m_CustomFence.IsFenceComplete(fenceValue);
	}

	void TaskFiber::SetCommandQueue(ID3D12CommandQueue* pCommandQueue, D3D12_COMMAND_LIST_TYPE type)
	{
		ASSERT(pCommandQueue);

		D3D12_COMMAND_QUEUE_DESC desc = pCommandQueue->GetDesc();
		ASSERT(desc.Type == type);

		m_pCommandQueue = pCommandQueue;
		m_CustomFence.SetCommandQueue(pCommandQueue);
	}

	bool TaskFiber::ResetAllocatorLists(size_t NumPair, ID3D12CommandAllocator** pAllocatorArray, ID3D12GraphicsCommandList** pListArray, D3D12_COMMAND_LIST_TYPE type)
	{
		bool bSafe = IsCompletedTask();

		if (bSafe)
		{
			ASSERT(m_CommandAllocators.size() == m_CommandLists.size());

			ReleaseCommandPair(m_CommandAllocators.size());

			m_CommandAllocators.reserve(NumPair);
			m_CommandLists.reserve(NumPair);

			m_CommandAllocators.insert(m_CommandAllocators.end(), pAllocatorArray, pAllocatorArray + NumPair);
			m_CommandLists.insert(m_CommandLists.end(), pListArray, pListArray + NumPair);
		}

		return bSafe;
	}
	void TaskFiber::PushbackAllocatorLists(size_t NumPair, ID3D12CommandAllocator** pAllocatorArray, ID3D12GraphicsCommandList** pListArray, D3D12_COMMAND_LIST_TYPE type)
	{
		m_CommandAllocators.reserve(m_CommandAllocators.size() + NumPair);
		m_CommandLists.reserve(m_CommandLists.size() + NumPair);

		m_CommandAllocators.insert(m_CommandAllocators.end(), pAllocatorArray, pAllocatorArray + NumPair);
		m_CommandLists.insert(m_CommandLists.end(), pListArray, pListArray + NumPair);
	}

	HRESULT TaskFiber::CloseLists()
	{
		for (auto& e : m_CommandLists)
		{
			ASSERT_HR(e->Close(), "Maybe you didn't call Reset().");
		}

		return S_OK;
	}

	uint64_t TaskFiber::ExecuteCommandQueue()
	{
		uint64_t NextCPUFenceValue = m_CustomFence.GetCPUSideNextFenceValue(m_Type);

		m_pCommandQueue->ExecuteCommandLists((UINT)m_CommandLists.size(), (ID3D12CommandList* const*)m_CommandLists.data());
		ASSERT_HR(m_pCommandQueue->Signal(m_CustomFence.GetFence(), NextCPUFenceValue));
		m_CustomFence.SetLastExecuteFenceValue(NextCPUFenceValue);
		return CustomFence::IncrementCPUFenceValue(m_Type);
	}

	HRESULT TaskFiber::Reset()
	{
		size_t NumAllocators = m_CommandAllocators.size();
		size_t NumLists      = m_CommandLists.size();
		
		ASSERT(NumAllocators == NumLists);

		for (size_t i = 0; i < NumLists; ++i)
		{
			ASSERT_HR(m_CommandAllocators[i]->Reset());
			ASSERT_HR(m_CommandLists[i]->Reset(m_CommandAllocators[i], nullptr));
		}

		return S_OK;
	}

	void TaskFiber::ReleaseCommandPair(size_t numPair)
	{
		size_t NumAllocators = m_CommandAllocators.size();
		size_t NumLists      = m_CommandLists.size();

		ASSERT(m_pOwningPool);
		ASSERT(NumAllocators == NumLists);
		ASSERT(m_CommandAllocators.size() < numPair);

		for (size_t i = NumLists - numPair; i < NumLists; ++i)
		{
			m_pOwningPool->ReleaseCommandAllocator(m_CommandAllocators[i]);
			m_pOwningPool->ReleaseCommandList(m_CommandLists[i]);
		}

		m_CommandAllocators.erase(m_CommandAllocators.end() - numPair, m_CommandAllocators.end());
		m_CommandAllocators.shrink_to_fit();

		m_CommandLists.erase(m_CommandLists.end() - numPair, m_CommandLists.end());
		m_CommandLists.shrink_to_fit();
	}
}

namespace custom
{
	TaskFiberManager* TaskFiberManager::s_pTaskFiberManager = nullptr;
	TaskFiberManager::TaskFiberManager()
		:
		m_pDevice(nullptr),
		m_CS({})
	{
		ASSERT(s_pTaskFiberManager == nullptr);
		s_pTaskFiberManager = this;
		InitializeCriticalSection(&m_CS);
	}
	TaskFiberManager::TaskFiberManager(ID3D12Device* pDevice)
		:
		m_pDevice(pDevice),
		m_CS({})
	{
		ASSERT(pDevice);
		ASSERT(s_pTaskFiberManager == nullptr);

		s_pTaskFiberManager = this;
		InitializeCriticalSection(&m_CS);

		m_QALPool[0].Create(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_QALPool[1].Create(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		m_QALPool[2].Create(pDevice, D3D12_COMMAND_LIST_TYPE_COPY);
	}

	void TaskFiberManager::Create(ID3D12Device* pDevice)
	{
		ASSERT(pDevice);
		m_pDevice = pDevice;
		m_QALPool[0].Create(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_QALPool[1].Create(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		m_QALPool[2].Create(pDevice, D3D12_COMMAND_LIST_TYPE_COPY);
	}

	void TaskFiberManager::Shutdown()
	{
		for (size_t i = 0; i < _countof(m_ProcessingTask); ++i)
		{
			for (auto& e : m_ProcessingTask[i])
			{
				delete e;
			}
		}

		if (m_CS.DebugInfo != 0)
		{
			DeleteCriticalSection(&m_CS);
			m_CS = {};
		}
	}

	STATIC TaskFiber* TaskFiberManager::RequestWorkSubmission(size_t numPairs, D3D12_COMMAND_LIST_TYPE type, uint64_t completedFenceValue /*= 0*/)
	{
		ASSERT(CHECK_VALID_TYPE(type));

		TaskFiber* pTaskFiber = nullptr;
		std::vector<TaskFiber*>& ProcessingTasks = s_pTaskFiberManager->m_ProcessingTask[TYPE_TO_INDEX(type)];

		size_t NumCommandAllocators = 0ul;
		size_t NumCommandLists      = 0ul;

		for (auto& e : ProcessingTasks)
		{
			if (e->IsCompletedTask(completedFenceValue))
			{
				NumCommandAllocators = e->GetNumCommandAllocators();
				NumCommandLists      = e->GetNumCommandLists();

				ASSERT(NumCommandAllocators == NumCommandLists);

				if (numPairs < NumCommandLists)
				{
					e->ReleaseCommandPair(NumCommandLists - numPairs);
				}
				else if (NumCommandLists < numPairs)
				{
					size_t NeedNumPairs = numPairs - NumCommandLists;

					ID3D12CommandAllocator** pAllocatorArray = (ID3D12CommandAllocator**)malloc(sizeof(ID3D12CommandAllocator*) * NeedNumPairs);
					ID3D12GraphicsCommandList** pListArray = (ID3D12GraphicsCommandList**)malloc(sizeof(ID3D12GraphicsCommandList*) * NeedNumPairs);

					s_pTaskFiberManager->RequestCommandAllocatorsLists(type, NeedNumPairs, &pAllocatorArray, &pListArray);

					e->PushbackAllocatorLists(NeedNumPairs, pAllocatorArray, pListArray, type);

					free(pAllocatorArray);
					free(pListArray);
				}

				pTaskFiber = e;
			}
		}

		if (pTaskFiber == nullptr)
		{
			CustomCommandQALPool* pPool = s_pTaskFiberManager->GetQALPool(type);

			pTaskFiber = new TaskFiber(pPool, pPool->RequestCommandQueue(), type);
			
			ID3D12CommandAllocator**    pAllocatorArray = (ID3D12CommandAllocator**)   malloc(sizeof(ID3D12CommandAllocator*)    * numPairs);
			ID3D12GraphicsCommandList** pListArray      = (ID3D12GraphicsCommandList**)malloc(sizeof(ID3D12GraphicsCommandList*) * numPairs);

			s_pTaskFiberManager->RequestCommandAllocatorsLists(type, numPairs, &pAllocatorArray, &pListArray);

			s_pTaskFiberManager->m_ProcessingTask[TYPE_TO_INDEX(type)].push_back(pTaskFiber);

			free(pAllocatorArray);
			free(pListArray);
		}

		return pTaskFiber;
	}

	STATIC void TaskFiberManager::Release(TaskFiber* pWorkSubmission)
	{
		// 일단 CPUWAIT개념은 안 잃는걸로.
		ASSERT(pWorkSubmission->IsCompletedTask());

		D3D12_COMMAND_LIST_TYPE Type = pWorkSubmission->GetCommandType();

		s_pTaskFiberManager->m_QALPool[Type].ReleaseCommandQueue(pWorkSubmission->GetCommandQueue());
		s_pTaskFiberManager->m_QALPool[Type].ReleaseCommandAllocators(pWorkSubmission->m_CommandAllocators.data(), pWorkSubmission->GetNumCommandAllocators());
		s_pTaskFiberManager->m_QALPool[Type].ReleaseCommandLists(pWorkSubmission->m_CommandLists.data(), pWorkSubmission->GetNumCommandLists());

		pWorkSubmission->m_pCommandQueue = nullptr;
		pWorkSubmission->m_CommandAllocators.clear();
		pWorkSubmission->m_CommandAllocators.shrink_to_fit();
		pWorkSubmission->m_CommandLists.clear();
		pWorkSubmission->m_CommandLists.shrink_to_fit();
	}

	CustomCommandQALPool* TaskFiberManager::GetQALPool(D3D12_COMMAND_LIST_TYPE type)
	{
		ASSERT(CHECK_VALID_TYPE(type));

		return &m_QALPool[TYPE_TO_INDEX(type)];
	}

	void TaskFiberManager::RequestCommandAllocatorsLists
	(
		D3D12_COMMAND_LIST_TYPE type, 
		size_t numLists, 
		ID3D12CommandAllocator*** p_pCommandAllocatorArray, 
		ID3D12GraphicsCommandList*** p_pListArray
	)
	{
		ASSERT(CHECK_VALID_TYPE(type));

		m_QALPool[TYPE_TO_INDEX(type)].RequestCommandAllocatorsLists(numLists, p_pCommandAllocatorArray, p_pListArray);
	}


	// 지금 의식의 흐름.
	// 1. RequestWorkSubmission을 하면 포인터만 넘겨주고 싶다.
	// 2. 근데 여기서 문제는 포인터만 주면 std::vector가 확장되면,
	// 3. std::vector가 컨테이너(배열)을 복사해서 옮기면
	// 컨테이너의 원소의 주소는 변하게 되는데,
	// 다행히 CommandQueue,Fence,CommandAllocator,CommandList는 포인터변수라서
	// 안변하는데 bool변수는 바뀌게되서 나중에 확장되면 참조하지 못하게됨

	// 그러면 std::vector<TaskFiber*>으로 바꾸면 되긴하는데, 
	// 메모리 파편화가 심해진다... 그래서 MemoryPool로 바꾸고싶다.
}