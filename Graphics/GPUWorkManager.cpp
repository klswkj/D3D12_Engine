#include "stdafx.h"
#include "GPUWorkManager.h"

namespace custom
{
	TaskFiber::TaskFiber(ID3D12CommandQueue* pCommandQueue, D3D12_COMMAND_LIST_TYPE type, ID3D12Fence* pFence, uint64_t CPUFenceValue)
		:
		m_Type(type),
		m_CustomFence(type)
	{
		ASSERT(pCommandQueue);
		ASSERT(pFence);
		ASSERT
		(
			type == D3D12_COMMAND_LIST_TYPE_DIRECT ||
			type == D3D12_COMMAND_LIST_TYPE_COPY   ||
			type == D3D12_COMMAND_LIST_TYPE_COMPUTE
		);
		ASSERT((D3D12_COMMAND_LIST_TYPE)(CPUFenceValue >> 56) == type);

		m_pCommandQueue = pCommandQueue;
		m_CustomFence.Reset(pFence, m_Type, CPUFenceValue);
	}

	bool TaskFiber::IsCompletedTask(uint64_t fenceValue = 0)
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
	void TaskFiber::SetFence(ID3D12Fence* pFence, D3D12_COMMAND_LIST_TYPE type, uint64_t CPUSideNextFenceValue)
	{
		ASSERT(pFence);
		ASSERT(m_Type == type);

		m_CustomFence.Reset(pFence, type, CPUSideNextFenceValue);
	}
	bool TaskFiber::ResetAllocatorLists(size_t NumPair, ID3D12CommandAllocator** pAllocatorArray, ID3D12GraphicsCommandList** pListArray, D3D12_COMMAND_LIST_TYPE type)
	{
		bool bSafe = IsCompletedTask();

		if (bSafe)
		{
			m_CommandAllocators.clear();
			m_CommandLists.clear();
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
			ASSERT_HR(e->Close());
		}
	}

	uint64_t TaskFiber::ExecuteCommandQueue()
	{
		m_pCommandQueue->ExecuteCommandLists(m_CommandLists.size(), (ID3D12CommandList* const*)m_CommandLists.data());
	}

	HRESULT TaskFiber::Reset()
	{
		ASSERT(m_CommandAllocators.size() == m_CommandLists.size());

		size_t ElementSize = m_CommandAllocators.size();

		for (size_t i = 0; i < ElementSize; ++i)
		{
			ASSERT(m_CommandAllocators[i]->Reset());
			ASSERT(m_CommandLists[i]->Reset(m_CommandAllocators[i], nullptr));
		}
	}
}

namespace custom
{
	TaskFiberManager::TaskFiberManager(ID3D12Device* pDevice)
		:
		m_pDevice(pDevice)
	{
		ASSERT(pDevice);
		m_QALPool[0].Create(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_QALPool[1].Create(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		m_QALPool[2].Create(pDevice, D3D12_COMMAND_LIST_TYPE_COPY);
	}
	TaskFiberManager::~TaskFiberManager()
	{
		// Destoryer Calls
		// m_DirectQALPool.Shutdown();
		// m_CopyQALPool.Shutdown();
		// m_ComputeQALPool.Shutdown();
		// m_CustomFencePool.Shutdown();
	}

	// 근데 Allocate해주고, 
	// CommandQueue랑, Fence주잖아
	// 근데 쓰고 받았는데, 그게 Fence 안끝났는지도 확인해야됨.
	TaskFiber* TaskFiberManager::RequestWorkSubmission(size_t numPairs, D3D12_COMMAND_LIST_TYPE type, uint64_t completedFenceValue)
	{
		ASSERT
		(
			(type == D3D12_COMMAND_LIST_TYPE_DIRECT) ||
			(type == D3D12_COMMAND_LIST_TYPE_COPY)   ||
			(type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
		);

		TaskFiber* pTaskFiber = nullptr;
		std::vector<TaskFiber>& ProcessingTasks = m_ProcessingTask[(size_t)type / 2];
		CustomCommandQALPool& Pool = m_QALPool[(size_t)type / 2];
		
		// 일단 ProcessingTask중에 끝난게 있는지 확인.
		// 끝난게 있으면, 그 중에서 요구한 numPairs만큼 맞춰주고, 나머지는 Pool에 반환
		for (auto& e : ProcessingTasks)
		{
			if (e.IsCompletedTask())
			{

			}
		}

	}

	void TaskFiberManager::Release(TaskFiber* pWorkSubmission)
	{

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